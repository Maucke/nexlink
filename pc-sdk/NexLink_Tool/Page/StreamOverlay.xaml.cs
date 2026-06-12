using NexLink;
using System;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
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
        [DllImport("user32.dll")]
        static extern IntPtr GetDC(IntPtr hwnd);
        [DllImport("user32.dll")]
        static extern int ReleaseDC(IntPtr hwnd, IntPtr dc);
        [DllImport("gdi32.dll")]
        static extern bool BitBlt(IntPtr hdc, int nXDest, int nYDest, int nWidth, int nHeight,
            IntPtr hdcSrc, int nXSrc, int nYSrc, uint dwRop);

        private readonly NexLinkDevice _device;
        private readonly int _targetW;
        private readonly int _targetH;

        private CancellationTokenSource _cts;
        private int _frameCount;
        private readonly Stopwatch _sw = Stopwatch.StartNew();

        // Cached window rect (updated on UI thread during drag, read by worker thread)
        private volatile int _cachedX, _cachedY, _cachedW, _cachedH;
        private const int Inset = 28;
        private readonly double _dpi;

        private System.Windows.Point _dragStart;
        private bool _isDragging;

        public StreamOverlay(NexLinkDevice device, DisplayInfo displayInfo)
        {
            InitializeComponent();

            _device = device;
            _targetW = displayInfo.Width;
            _targetH = displayInfo.Height;

            using (var g = Graphics.FromHwnd(IntPtr.Zero))
                _dpi = g.DpiX / 96.0;

            var screenW = (int)(SystemParameters.PrimaryScreenWidth / _dpi);
            var screenH = (int)(SystemParameters.PrimaryScreenHeight / _dpi);
            Left = (screenW - Width) / 2;
            Top = (screenH - Height) / 2;

            // Cache initial rect at native pixel coords
            UpdateCache();

            CloseBtn.Click += (s, e) => Stop();
            this.PreviewKeyDown += (s, e) => { if (e.Key == Key.Escape) Stop(); };

            this.MouseLeftButtonDown += OnTitleBarMouseDown;
            this.MouseMove += OnTitleBarMouseMove;
            this.MouseLeftButtonUp += OnTitleBarMouseUp;
            this.SizeChanged += OnSizeChanged;
            this.Loaded += (s, e) => StartCapture();
        }

        private void UpdateCache()
        {
            _cachedX = (int)((Left + Inset) * _dpi);
            _cachedY = (int)((Top + Inset) * _dpi);
            _cachedW = (int)((Width - Inset * 2) * _dpi);
            _cachedH = (int)((Height - Inset * 2) * _dpi);
        }

        private void OnSizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateCache();
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

            // Update cached rect as window moves
            UpdateCache();
        }

        private void OnTitleBarMouseUp(object sender, MouseButtonEventArgs e)
        {
            _isDragging = false;
            ReleaseMouseCapture();
        }

        private void StartCapture()
        {
            _cts = new CancellationTokenSource();
            var token = _cts.Token;

            // UI timer to show FPS
            var fpsTimer = new DispatcherTimer();
            fpsTimer.Interval = TimeSpan.FromMilliseconds(250);
            fpsTimer.Tick += (s, e) =>
            {
                double fps = _frameCount / _sw.Elapsed.TotalSeconds;
                InfoText.Text = $"{_targetW}x{_targetH}  {fps:F1} fps";
            };
            fpsTimer.Start();

            Task.Run(() =>
            {
                int bufSize = _targetW * _targetH * 2;

                while (!token.IsCancellationRequested)
                {
                    int x = _cachedX, y = _cachedY, w = _cachedW, h = _cachedH;
                    if (w < 4 || h < 4) { Thread.Sleep(10); continue; }

                    IntPtr dc = GetDC(IntPtr.Zero);
                    if (dc == IntPtr.Zero) continue;

                    Bitmap bmp = new Bitmap(w, h, PixelFormat.Format24bppRgb);
                    using (Graphics g = Graphics.FromImage(bmp))
                    {
                        IntPtr hdc = g.GetHdc();
                        BitBlt(hdc, 0, 0, w, h, dc, x, y, 0x00CC0020);
                        g.ReleaseHdc(hdc);
                    }
                    ReleaseDC(IntPtr.Zero, dc);

                    byte[] rgb565 = new byte[bufSize];
                    var srcRect = new Rectangle(0, 0, w, h);
                    var srcData = bmp.LockBits(srcRect, ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);
                    int srcStride = srcData.Stride;

                    unsafe
                    {
                        byte* srcBase = (byte*)srcData.Scan0;
                        int dstIdx = 0;
                        for (int ty = 0; ty < _targetH; ty++)
                        {
                            int sy = ty * h / _targetH;
                            byte* srcRow = srcBase + sy * srcStride;
                            for (int tx = 0; tx < _targetW; tx++)
                            {
                                int sx = tx * w / _targetW;
                                byte* pixel = srcRow + sx * 3;
                                ushort c = (ushort)(((pixel[2] >> 3) << 11) | ((pixel[1] >> 2) << 5) | (pixel[0] >> 3));
                                rgb565[dstIdx++] = (byte)(c >> 8);
                                rgb565[dstIdx++] = (byte)(c);
                            }
                        }
                    }
                    bmp.UnlockBits(srcData);
                    bmp.Dispose();

                    try
                    {
                        var result = new ImageResult(_targetW, _targetH, rgb565);
                        _device.SendFrame(result, TargetPixelFormat.Rgb565);
                    }
                    catch { }

                    _frameCount++;
                }
            }, token);
        }

        public void Stop()
        {
            _cts?.Cancel();
            if (Dispatcher.CheckAccess())
                Close();
            else
                Dispatcher.BeginInvoke(new Action(Close));
        }

        protected override void OnClosed(EventArgs e)
        {
            _cts?.Cancel();
            base.OnClosed(e);
        }
    }
}
