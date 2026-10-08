using NexLink;
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

namespace NexLink_Tool.Page
{
    public partial class StreamOverlay : Window
    {
        private readonly NexLinkDevice _device;
        private readonly int _targetW;
        private readonly int _targetH;
        private readonly bool _useCamera;

        private readonly ScreenFrameSource _screenSource = new ScreenFrameSource();
        private IFrameSource _source;

        private CancellationTokenSource _cts;
        private Task _sendTask;
        private int _frameCount;
        private readonly Stopwatch _sw = Stopwatch.StartNew();

        private const int Inset = 28;
        private readonly double _dpi;

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
            _screenSource.Region = new Rectangle(
                (int)((Left + 2) * _dpi),
                (int)((Top + Inset) * _dpi),
                (int)((Width - 4) * _dpi),
                (int)((Height - Inset * 2) * _dpi));
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
            if (_useCamera)
            {
                var camera = new CameraFrameSource();
                string error = camera.Start();
                if (error != null)
                {
                    InfoText.Text = error;
                    return;
                }
                _source = camera;
            }
            else
            {
                _source = _screenSource;
            }

            StartWorkers(_useCamera ? "Camera" : "Screen");
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
            _sendTask = Task.Run(() => SendLoop(token), token);
        }

        private void CaptureLoop(CancellationToken token)
        {
            int bufSize = _targetW * _targetH * 2;
            IFrameSource source = _source;
            bool mirror = source.MirrorHorizontally;

            while (!token.IsCancellationRequested)
            {
                Bitmap bmp = source.GetFrame();
                if (bmp == null) { Thread.Sleep(10); continue; }

                byte[] rgb565 = new byte[bufSize];
                int w = bmp.Width;
                int h = bmp.Height;
                int cropX = 0;
                int cropY = 0;
                int cropW = w;
                int cropH = h;
                if (source.CropToTargetAspect)
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
                            int offset = tx * cropW / _targetW;
                            int sx = mirror ? cropX + cropW - 1 - offset : cropX + offset;
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

            // 停流/关窗后把设备屏幕刷黑，否则设备会一直停在最后一帧画面上。
            // 收尾放在这个发送线程里做（而不是调用方），是为了保证此刻没有别的
            // 线程在往同一个设备发帧。
            try
            {
                _device.ClearScreen(_targetW, _targetH);
            }
            catch
            {
            }
        }

        /// <summary>
        /// 等发送线程收尾（包含它退出前发的黑屏帧）。
        /// 退出程序前用它，保证黑屏是设备收到的最后一帧。返回是否已经结束。
        /// </summary>
        public bool WaitStopped(int timeoutMs)
        {
            var t = _sendTask;
            return t == null || t.Wait(timeoutMs);
        }

        public void Stop()
        {
            _cts?.Cancel();
            _frameQueue.CompleteAdding();
            _source?.Stop();
            if (Dispatcher.CheckAccess())
                Close();
            else
                Dispatcher.BeginInvoke(new Action(Close));
        }

        protected override void OnClosed(EventArgs e)
        {
            _cts?.Cancel();
            _frameQueue.CompleteAdding();
            _source?.Stop();
            base.OnClosed(e);
        }
    }
}
