using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace NexLink_Tool.Page
{
    /// <summary>Captures the desktop area under the overlay window via BitBlt.</summary>
    public sealed class ScreenFrameSource : IFrameSource
    {
        [DllImport("user32.dll")]
        private static extern IntPtr GetDC(IntPtr hwnd);
        [DllImport("user32.dll")]
        private static extern int ReleaseDC(IntPtr hwnd, IntPtr dc);
        [DllImport("gdi32.dll")]
        private static extern bool BitBlt(IntPtr hdc, int nXDest, int nYDest, int nWidth, int nHeight,
            IntPtr hdcSrc, int nXSrc, int nYSrc, uint dwRop);

        private const uint SrcCopy = 0x00CC0020;

        /// <summary>Area to capture in physical pixels; the overlay updates it as its window moves.</summary>
        public Rectangle Region { get; set; }

        public bool MirrorHorizontally => false;
        public bool CropToTargetAspect => false;

        public Bitmap GetFrame()
        {
            var r = Region;
            if (r.Width < 4 || r.Height < 4)
                return null;

            IntPtr dc = GetDC(IntPtr.Zero);
            if (dc == IntPtr.Zero)
                return null;

            try
            {
                var bmp = new Bitmap(r.Width, r.Height, PixelFormat.Format24bppRgb);
                using (Graphics g = Graphics.FromImage(bmp))
                {
                    IntPtr hdc = g.GetHdc();
                    try
                    {
                        BitBlt(hdc, 0, 0, r.Width, r.Height, dc, r.X, r.Y, SrcCopy);
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

        public void Stop()
        {
        }
    }
}
