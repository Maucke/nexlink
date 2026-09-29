using NexLink;
using AForge.Video;
using AForge.Video.DirectShow;
using System;
using System.Collections.Concurrent;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using ImageBppConverter;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;
using System.Runtime.InteropServices;

namespace NexLink_Tool.Page
{
    public partial class StreamOverlay : Window
    {
        [DllImport("user32.dll")]
        private static extern IntPtr GetDC(IntPtr hwnd);
        [DllImport("user32.dll")]
        private static extern int ReleaseDC(IntPtr hwnd, IntPtr dc);
        [DllImport("gdi32.dll")]
        private static extern bool BitBlt(IntPtr hdc, int nXDest, int nYDest, int nWidth, int nHeight,
            IntPtr hdcSrc, int nXSrc, int nYSrc, uint dwRop);

        private readonly NexLinkDevice _device;
        private readonly int _targetW;
        private readonly int _targetH;
        private readonly bool _useCamera;

        private CancellationTokenSource _cts;
        private int _frameCount;
        private readonly Stopwatch _sw = Stopwatch.StartNew();

        private const int Inset = 28;
        private readonly double _dpi;
        private volatile int _cachedX, _cachedY, _cachedW, _cachedH;
        private readonly object _cameraLock = new object();
        private VideoCaptureDevice _camera;
        private Bitmap _latestCameraFrame;

        private System.Windows.Point _dragStart;
        private bool _isDragging;

        // Thread-safe frame queue between capture and send
        private readonly BlockingCollection<byte[]> _frameQueue = new BlockingCollection<byte[]>(4);

        public StreamOverlay(NexLinkDevice device, DisplayInfo displayInfo)
            : this(device, displayInfo, false)
        {
        }

        public StreamOverlay(NexLinkDevice device, DisplayInfo displayInfo, bool useCamera)
        {
            InitializeComponent();

            _device = device;
            _targetW = displayInfo.Width;
            _targetH = displayInfo.Height;
            _useCamera = useCamera;

            using (var g = Graphics.FromHwnd(IntPtr.Zero))
                _dpi = g.DpiX / 96.0;

            double aspect = (double)_targetW / _targetH;
            Height = Math.Round((Width - 4) / aspect + 56);

            var screenW = (int)(SystemParameters.PrimaryScreenWidth / _dpi);
            var screenH = (int)(SystemParameters.PrimaryScreenHeight / _dpi);
            Left = (screenW - Width) / 2;
            Top = (screenH - Height) / 2;
            UpdateCache();

            CloseBtn.Click += (s, e) => Stop();
            this.PreviewKeyDown += (s, e) => { if (e.Key == Key.Escape) Stop(); };

            this.MouseLeftButtonDown += OnTitleBarMouseDown;
            this.MouseMove += OnTitleBarMouseMove;
            this.MouseLeftButtonUp += OnTitleBarMouseUp;
            this.SizeChanged += OnSizeChanged;
            this.Loaded += (s, e) => StartCapture();
        }

        private bool _isAdjusting;
        private void UpdateCache()
        {
            _cachedX = (int)((Left + 2) * _dpi);
            _cachedY = (int)((Top + Inset) * _dpi);
            _cachedW = (int)((Width - 4) * _dpi);
            _cachedH = (int)((Height - Inset * 2) * _dpi);
        }

        private void OnSizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (_isAdjusting) return;
            _isAdjusting = true;

            double aspect = (double)_targetW / _targetH;
            if (e.WidthChanged)
            {
                double clientW = Width - 4;
                Height = Math.Round(clientW / aspect + 56);
            }
            else if (e.HeightChanged)
            {
                double clientH = Height - 56;
                Width = Math.Round(clientH * aspect + 4);
            }

            UpdateCache();
            _isAdjusting = false;
        }

        private void OnTitleBarMouseDown(object sender, MouseButtonEventArgs e)
        {
            var pos = e.GetPosition(this);
            if (pos.Y < 30)
            {
                _isDragging = true;
                _dragStart = pos;
                CaptureMouse();
            }
        }

        private void OnTitleBarMouseMove(object sender, MouseEventArgs e)
        {
            if (!_isDragging) return;
            var pos = e.GetPosition(this);
            Left += pos.X - _dragStart.X;
            Top += pos.Y - _dragStart.Y;
            UpdateCache();
        }

        private void OnTitleBarMouseUp(object sender, MouseButtonEventArgs e)
        {
            _isDragging = false;
            ReleaseMouseCapture();
        }

        private void StartCapture()
        {
            if (!_useCamera)
            {
                StartWorkers("Screen");
                return;
            }

            try
            {
                var cameras = new FilterInfoCollection(FilterCategory.VideoInputDevice);
                if (cameras.Count == 0)
                {
                    InfoText.Text = "No camera found";
                    return;
                }

                _camera = new VideoCaptureDevice(cameras[0].MonikerString);
                _camera.NewFrame += OnCameraFrame;
                _camera.Start();
            }
            catch (Exception ex)
            {
                InfoText.Text = $"Camera failed: {ex.Message}";
                StopCamera();
                return;
            }

            StartWorkers("Camera");
        }

        public void StartBackgroundCapture()
        {
            if (_useCamera)
                StartCapture();
        }

        private void StartWorkers(string sourceName)
        {
            _cts = new CancellationTokenSource();
            var token = _cts.Token;

            var fpsTimer = new DispatcherTimer();
            fpsTimer.Interval = TimeSpan.FromMilliseconds(250);
            fpsTimer.Tick += (s, e) =>
            {
                double fps = _frameCount / _sw.Elapsed.TotalSeconds;
                InfoText.Text = $"{sourceName}  {_targetW}x{_targetH}  {fps:F1} fps";
            };
            fpsTimer.Start();

            // Thread 1: capture + encode
            Task.Run(() => CaptureLoop(token), token);

            // Thread 2: USB send
            Task.Run(() => SendLoop(token), token);
        }

        private void CaptureLoop(CancellationToken token)
        {
            int bufSize = _targetW * _targetH * 2;

            while (!token.IsCancellationRequested)
            {
                Bitmap bmp = _useCamera ? GetLatestCameraFrame() : CaptureDesktopFrame();
                if (bmp == null) { Thread.Sleep(10); continue; }

                byte[] rgb565 = new byte[bufSize];
                int w = bmp.Width;
                int h = bmp.Height;
                int cropX = 0;
                int cropY = 0;
                int cropW = w;
                int cropH = h;
                if (_useCamera)
                {
                    double sourceAspect = (double)w / h;
                    double targetAspect = (double)_targetW / _targetH;
                    if (sourceAspect > targetAspect)
                    {
                        cropW = (int)(h * targetAspect);
                        cropX = (w - cropW) / 2;
                    }
                    else if (sourceAspect < targetAspect)
                    {
                        cropH = (int)(w / targetAspect);
                        cropY = (h - cropH) / 2;
                    }
                }
                var srcRect = new Rectangle(0, 0, w, h);
                var srcData = bmp.LockBits(srcRect, ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
                int srcStride = srcData.Stride;

                unsafe
                {
                    byte* srcBase = (byte*)srcData.Scan0;
                    int dstIdx = 0;
                    for (int ty = 0; ty < _targetH; ty++)
                    {
                        int sy = cropY + ty * cropH / _targetH;
                        byte* srcRow = srcBase + sy * srcStride;
                        for (int tx = 0; tx < _targetW; tx++)
                        {
                            int sx = cropX + (cropW - 1) - tx * cropW / _targetW;
                            byte* pixel = srcRow + sx * 3;
                            ushort c = (ushort)(((pixel[2] >> 3) << 11) | ((pixel[1] >> 2) << 5) | (pixel[0] >> 3));
                            rgb565[dstIdx++] = (byte)(c >> 8);
                            rgb565[dstIdx++] = (byte)(c);
                        }
                    }
                }
                bmp.UnlockBits(srcData);
                bmp.Dispose();

                // TryAdd: if queue is full (bound=4), discard oldest
                while (!_frameQueue.TryAdd(rgb565, 0, token))
                {
                    _frameQueue.TryTake(out _, 0, token);
                }

            }
        }

        private Bitmap CaptureDesktopFrame()
        {
            int x = _cachedX, y = _cachedY, w = _cachedW, h = _cachedH;
            if (w < 4 || h < 4)
                return null;

            IntPtr dc = GetDC(IntPtr.Zero);
            if (dc == IntPtr.Zero)
                return null;

            try
            {
                var bmp = new Bitmap(w, h, PixelFormat.Format24bppRgb);
                using (Graphics g = Graphics.FromImage(bmp))
                {
                    IntPtr hdc = g.GetHdc();
                    try
                    {
                        BitBlt(hdc, 0, 0, w, h, dc, x, y, 0x00CC0020);
                    }
                    finally
                    {
                        g.ReleaseHdc(hdc);
                    }
                }
                return bmp;
            }
            finally
            {
                ReleaseDC(IntPtr.Zero, dc);
            }
        }

        private void OnCameraFrame(object sender, NewFrameEventArgs e)
        {
            var frame = (Bitmap)e.Frame.Clone();
            lock (_cameraLock)
            {
                _latestCameraFrame?.Dispose();
                _latestCameraFrame = frame;
            }
        }

        private Bitmap GetLatestCameraFrame()
        {
            lock (_cameraLock)
                return _latestCameraFrame == null ? null : (Bitmap)_latestCameraFrame.Clone();
        }

        private void StopCamera()
        {
            if (_camera != null)
            {
                _camera.NewFrame -= OnCameraFrame;
                if (_camera.IsRunning)
                    _camera.SignalToStop();
                _camera.WaitForStop();
                _camera = null;
            }

            lock (_cameraLock)
            {
                _latestCameraFrame?.Dispose();
                _latestCameraFrame = null;
            }
        }

        private void SendLoop(CancellationToken token)
        {
            while (!token.IsCancellationRequested)
            {
                Interlocked.Increment(ref _frameCount);
                Thread.Sleep(1);
                try
                {
                    byte[] data = _frameQueue.Take(token);
                    var result = new ImageResult(_targetW, _targetH, data);
                    _device.SendFrameFast(result, TargetPixelFormat.Rgb565);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
                catch
                {
                }
            }
        }

        public void Stop()
        {
            _cts?.Cancel();
            _frameQueue.CompleteAdding();
            StopCamera();
            if (Dispatcher.CheckAccess())
                Close();
            else
                Dispatcher.BeginInvoke(new Action(Close));
        }

        protected override void OnClosed(EventArgs e)
        {
            _cts?.Cancel();
            _frameQueue.CompleteAdding();
            StopCamera();
            base.OnClosed(e);
        }
    }
}
