using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ImageBppConverter
{
    public enum TargetPixelFormat
    {
        Rgb888,
        Rgb565,
        Gray8,
        Mono1,
        Dual2Color,
        Dual2ColorGray8
    }

    public class ImageResult
    {
        public int Width { get; set; }
        public int Height { get; set; }
        public byte[] Data { get; set; }

        public ImageResult(int width, int height, byte[] data)
        {
            Width = width;
            Height = height;
            Data = data;
        }

        public override string ToString()
        {
            return $"{Width}x{Height}, DataLength={Data.Length} bytes";
        }
    }

    public static class ImageConverter
    {
        public static ImageResult Convert(Bitmap bmp, TargetPixelFormat format)
        {
            try
            {
                switch (format)
                {
                    case TargetPixelFormat.Rgb888:
                        return ConvertRgb888(bmp);

                    case TargetPixelFormat.Rgb565:
                        return ConvertRgb565(bmp);

                    case TargetPixelFormat.Gray8:
                        return ConvertGray8(bmp);

                    case TargetPixelFormat.Mono1:
                        return ConvertMono1(bmp);

                    case TargetPixelFormat.Dual2Color:
                        return ConvertDual2NoGray(bmp);

                    case TargetPixelFormat.Dual2ColorGray8:
                        return ConvertDual2ColorGray8(bmp);
                    default:
                        throw new NotSupportedException();
                }
            }
            finally
            {
                bmp.Dispose();
            }
        }


        private static byte[] GetRgbData(Bitmap bmp, out int stride)
        {
            var rect = new Rectangle(0, 0, bmp.Width, bmp.Height);
            var data = bmp.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format24bppRgb);

            stride = data.Stride;
            byte[] buffer = new byte[stride * bmp.Height];

            Marshal.Copy(data.Scan0, buffer, 0, buffer.Length);
            bmp.UnlockBits(data);

            return buffer;
        }
        private static ImageResult ConvertDual2NoGray(Bitmap bmp)
        {
            var rgb = GetRgbData(bmp, out int stride);

            int byteWidth = (bmp.Width + 3) / 4;
            byte[] output = new byte[byteWidth * bmp.Height];

            for (int y = 0; y < bmp.Height; y++)
            {
                for (int x = 0; x < bmp.Width; x++)
                {
                    int src = y * stride + x * 3;

                    byte b = rgb[src];
                    byte g = rgb[src + 1];
                    byte r = rgb[src + 2];

                    byte level = 0;

                    // ===== 颜色判定逻辑 =====
                    bool red = r > 128;
                    bool green = g > 128 || b > 128;

                    if (red && green)
                        level = 3;     // 双亮（如果支持）
                    else if (red)
                        level = 1;
                    else if (green)
                        level = 2;
                    else
                        level = 0;     // 黑

                    int byteIndex = y * byteWidth + (x / 4);
                    int shift = (3 - (x % 4)) * 2;

                    output[byteIndex] |= (byte)(level << shift);
                }
            }

            return new ImageResult(bmp.Width, bmp.Height, output);
        }

        private static ImageResult ConvertRgb888(Bitmap bmp)
        {
            var rgb = GetRgbData(bmp, out int stride);

            byte[] output = new byte[bmp.Width * bmp.Height * 3];
            int index = 0;

            for (int y = 0; y < bmp.Height; y++)
            {
                for (int x = 0; x < bmp.Width; x++)
                {
                    int src = y * stride + x * 3;

                    output[index++] = rgb[src + 2]; // R
                    output[index++] = rgb[src + 1]; // G
                    output[index++] = rgb[src];     // B
                }
            }

            return new ImageResult(bmp.Width, bmp.Height, output);
        }

        private static ImageResult ConvertRgb565(Bitmap bmp)
        {
            var rgb = GetRgbData(bmp, out int stride);

            byte[] output = new byte[bmp.Width * bmp.Height * 2];
            int index = 0;

            for (int y = 0; y < bmp.Height; y++)
            {
                for (int x = 0; x < bmp.Width; x++)
                {
                    int src = y * stride + x * 3;

                    byte b = rgb[src];
                    byte g = rgb[src + 1];
                    byte r = rgb[src + 2];

                    ushort rgb565 =
                        (ushort)(((r >> 3) << 11) |
                                 ((g >> 2) << 5) |
                                 (b >> 3));

                    output[index++] = (byte)(rgb565 >> 8);
                    output[index++] = (byte)(rgb565 & 0xFF);
                }
            }

            return new ImageResult(bmp.Width, bmp.Height, output);
        }

        private static ImageResult ConvertGray8(Bitmap bmp)
        {
            var rgb = GetRgbData(bmp, out int stride);

            byte[] output = new byte[bmp.Width * bmp.Height];
            int index = 0;

            for (int y = 0; y < bmp.Height; y++)
            {
                for (int x = 0; x < bmp.Width; x++)
                {
                    int src = y * stride + x * 3;

                    byte b = rgb[src];
                    byte g = rgb[src + 1];
                    byte r = rgb[src + 2];

                    output[index++] =
                        (byte)(0.299 * r + 0.587 * g + 0.114 * b);
                }
            }

            return new ImageResult(bmp.Width, bmp.Height, output);
        }

        private static ImageResult ConvertMono1(Bitmap bmp)
        {
            var grayResult = ConvertGray8(bmp);
            var gray = grayResult.Data;

            int byteWidth = (bmp.Width + 7) / 8;
            byte[] output = new byte[byteWidth * bmp.Height];

            for (int y = 0; y < bmp.Height; y++)
            {
                for (int x = 0; x < bmp.Width; x++)
                {
                    int grayIndex = y * bmp.Width + x;

                    if (gray[grayIndex] > 128)
                    {
                        int byteIndex = y * byteWidth + (x / 8);
                        output[byteIndex] |= (byte)(0x80 >> (x % 8));
                    }
                }
            }

            return new ImageResult(bmp.Width, bmp.Height, output);
        }

        public static Bitmap Convert2bppToBitmap(ImageResult image)
        {
            var width = image.Width;
            var height = image.Height;
            var data = image.Data;

            Bitmap bmp = new Bitmap(width, height);

            int byteWidth = (width + 3) / 4;
            int index = 0;

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x += 4)
                {
                    byte b = data[index++];

                    for (int i = 0; i < 4; i++)
                    {
                        int px = x + i;
                        if (px >= width)
                            continue;

                        byte color = (byte)((b >> (6 - i * 2)) & 0x03);

                        Color c = Color.Transparent;

                        switch (color)
                        {
                            case 1: c = Color.Orange; break;
                            case 2: c = Color.DeepSkyBlue; break;
                            case 3: c = Color.FromArgb(0xFF, 0xF0, 0xF0, 0xF0); break;
                        }

                        bmp.SetPixel(px, y, c);
                    }
                }
            }

            return bmp;
        }
        private static ImageResult ConvertDual2ColorGray8(Bitmap bmp)
        {
            var rgb = GetRgbData(bmp, out int stride);

            byte[] output = new byte[bmp.Width * bmp.Height];
            int index = 0;

            for (int y = 0; y < bmp.Height; y++)
            {
                for (int x = 0; x < bmp.Width; x++)
                {
                    Color c = bmp.GetPixel(x, y);

                    if (c.A == 0)
                    {
                        output[index++] = 0;
                        continue;
                    }

                    // ===== 反推 gray 值 =====
                    byte red_gray = (byte)Math.Round(c.R * 7.0 / 255.0);
                    byte blue_gray = (byte)Math.Round(c.B * 7.0 / 255.0);

                    // 限制 0~7
                    red_gray = (byte)Math.Min((byte)7, Math.Max((byte)0, red_gray));
                    blue_gray = (byte)Math.Min((byte)7, Math.Max((byte)0, blue_gray));

                    // ===== 打包 =====
                    byte packed = (byte)((red_gray << 4) | blue_gray);

                    output[index++] = packed;
                }
            }

            return new ImageResult(bmp.Width, bmp.Height, output);
        }
        public static Bitmap ConvertDual2ColorGray8ToBitmap(ImageResult image)
        {
            var width = image.Width;
            var height = image.Height;
            var data = image.Data;

            if (data.Length < width * height) return null;

            Bitmap bmp = new Bitmap(width, height);

            int index = 0;

            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < width; x++)
                {
                    byte value = data[index++];

                    byte red_gray = (byte)((value >> 4) & 0x7);
                    byte blue_gray = (byte)(value & 0x7);

                    int red_intensity = red_gray * 255 / 7;
                    int blue_intensity = blue_gray * 255 / 7;

                    Color c;

                    // 混色（橙 + Cyan）

                    // 橙色分量
                    int r1 = red_intensity;
                    int g1 = red_intensity * 232 / 255;
                    int b1 = 0;

                    // Cyan分量
                    int r2 = 0;
                    int g2 = blue_intensity * 200 / 255;
                    int b2 = blue_intensity;

                    // 叠加并限制范围
                    int r = Math.Min(Math.Max(r1, r2), 255);
                    int g = Math.Min(Math.Max(g1, g2), 255);
                    int b = Math.Min(Math.Max(b1, b2), 255);

                    c = Color.FromArgb(r, g, b);

                    if (c == Color.FromArgb(0, 0, 0))
                           c = Color.Transparent;
                        //  c = Color.FromArgb(0x2E, 0x25, 0x27);

                    bmp.SetPixel(x, y, c);
                }
            }

            return bmp;
        }
    }
}
