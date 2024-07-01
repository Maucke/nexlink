using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using WMPLib;

namespace NetLink_MediaPlayer
{
    public class NexLink
    {
        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int init();

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int scandevices();

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int initwithindex(int index);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void close();

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int receive(byte[] data, int length);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int transfer(byte[] data, int length);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int control_get(byte bRequest, ushort wValue, byte[] data, ushort wLength);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int control_set(byte bRequest, ushort wValue, byte[] data, ushort wLength);
    }

    public enum NEX_BREQ : Byte
    {
        NEX_BREQ_HOST_FORMAT = 0,
        NEX_TIMESTAMP_SET,
        NEX_TIMESTAMP_GET,
        NEX_BRIGHTNESS_SET,
        NEX_BRIGHTNESS_GET,
        NEX_SCREEN_SET,
        NEX_SCREEN_GET,
        NEX_COMMAND_LEN,
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_brightness_des
    {
        public UInt16 brightness;
        public UInt16 damp;
    };


    public struct nex_usb_des
    {
        public UInt64 timestamp_s;
        public nex_brightness_des brides;
    };


    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
           // this.Topmost = true;
        }

        //const int SCR_WIDTH = 160;
        //const int SCR_HEIGHT = 128;
        //const int SCR_HEIGHT = 240;
        //const int SCR_WIDTH = 280;
        const int SCR_HEIGHT = 280;
        const int SCR_WIDTH = 240;
        const int BLOK_VALID = 960;

        public byte[] StructToBytes(object odata)
        {
            int size = Marshal.SizeOf(odata);
            byte[] byteArray = new byte[size];

            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.StructureToPtr(odata, ptr, false);
                Marshal.Copy(ptr, byteArray, 0, size);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }

            return byteArray;
        }

        public object BytesToStruct(byte[] byteArray, Type type)
        {
            int size = Marshal.SizeOf(type);
            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.Copy(byteArray, 0, ptr, size);
                return (object)Marshal.PtrToStructure(ptr, type);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        public int SetDirection(byte dir)
        {
            var rawdata = new byte[1];
            rawdata[0] = (byte)((dir & 3));
            return NexLink.control_set((byte)NEX_BREQ.NEX_SCREEN_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public int SetTimestamp()
        {
            DateTime currentTime = DateTime.Now;
            DateTime unixStartTime = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Local);
            TimeSpan elapsedTime = currentTime - unixStartTime;
            long timestamp = (long)elapsedTime.TotalSeconds;

            var rawdata = BitConverter.GetBytes(timestamp);
            return NexLink.control_set((byte)NEX_BREQ.NEX_TIMESTAMP_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public int GetTimestamp(ref long timestamp)
        {
            var rawdata = new byte[BitConverter.GetBytes(timestamp).Length];
            var ret = NexLink.control_get((byte)NEX_BREQ.NEX_TIMESTAMP_GET, 0, rawdata, (ushort)rawdata.Length);
            timestamp = BitConverter.ToInt64(rawdata, 0);
            return ret;
        }

        public int SetBrightness(nex_brightness_des brides)
        {
            var rawdata = StructToBytes(brides);
            return NexLink.control_set((byte)NEX_BREQ.NEX_BRIGHTNESS_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public int GetBrightness(ref nex_brightness_des brides)
        {
            var rawdata = new byte[StructToBytes(brides).Length];
            var ret = NexLink.control_get((byte)NEX_BREQ.NEX_BRIGHTNESS_GET, 0, rawdata, (ushort)rawdata.Length);
            brides = (nex_brightness_des)BytesToStruct(rawdata, typeof(nex_brightness_des));
            return ret;
        }


        int fps = 0;
        int picfps = 0;
        byte[] rawdata;
        bool usbalive = false;
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            AxWMPLib.AxWindowsMediaPlayer player = frm_media;
            player.enableContextMenu = false;
            player.URL = @"E:\Downloads\test.mp3"; // 替换为你想要播放的音频文件路径
            player.settings.enableErrorDialogs = true;
            player.settings.autoStart = false;
            player.settings.volume = 50;

            // 设置可视化效果类型
            player.settings.setMode("autoRewind", true);
            //player.settings.setMode("loop", true);
            //player.settings.setMode("shuffle", false);
            player.StatusChange += Player_StatusChange;
            // 显示可视化效果
            player.uiMode = "none";
            player.stretchToFit = true;

            //var json = MusicAPI.Search("执迷", 5);
            //Console.WriteLine(json);
        }

        private void Player_StatusChange(object sender, EventArgs e)
        {
        }

        private void Dispatcher_Tick(object sender, EventArgs e)
        {
            // 捕获屏幕指定区域的图像
            var point = frm_media.PointToScreen(System.Drawing.Point.Empty);
            System.Drawing.Rectangle rectangle = new System.Drawing.Rectangle(point.X, point.Y, frm_media.Bounds.Width, frm_media.Bounds.Height);
            try
            {
                var data = CaptureScreenPart(rectangle);
                rawdata = data;
                picfps++;
            }
            catch (Exception)
            { 
            
            }
        }

        public void SendOnPic(byte[] data)
        {
            var recvdata = new byte[1024];
            for (int i = 0; i < (SCR_WIDTH * SCR_HEIGHT * 2) / BLOK_VALID; i++)
            {
                for (int p = 0; p < BLOK_VALID; p++)
                {
                    if (p % 2 == 0)
                    {
                            recvdata[p] = data[i * BLOK_VALID + p + 1];
                    }
                    else
                    {
                            recvdata[p] = data[i * BLOK_VALID + p - 1];
                    }
                }
                NexLink.transfer(recvdata, 1024);
            }
        }

        public void Test(byte[] data)
        {
            long index = 0;
            var recvdata = new byte[1024];
            for (int i = 0; i < (SCR_WIDTH * SCR_HEIGHT * 2) / 960; i++)
            {
                for (int j = 0; j < 2; j++)
                {
                    for (int p = 0; p < 480; p++)
                    {
                        recvdata[p + j * 480] = (byte)((index++) % (960 * 2)/ 960 == 1 ? 0xFF : 0);
                    }
                }
                NexLink.transfer(recvdata, 1024);
            }
        }

        public byte[] ConvertTo16BitByteArray(Bitmap bitmap)
        {
            // 将图像转换为16位RGB565格式
            BitmapData bmpData = bitmap.LockBits(new System.Drawing.Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, System.Drawing.Imaging.PixelFormat.Format16bppRgb565);
            int byteCount = bmpData.Stride * bitmap.Height;
            byte[] byteArray = new byte[byteCount];
            IntPtr ptr = bmpData.Scan0;

            // 将像素数据复制到字节数组中
            System.Runtime.InteropServices.Marshal.Copy(ptr, byteArray, 0, byteCount);

            bitmap.UnlockBits(bmpData);

            return byteArray;
        }

        Graphics graphics;

        public byte[] CaptureScreenPart(System.Drawing.Rectangle bounds)
        {
            // 创建一个与控件大小相同的位图
            Bitmap bitmap = new Bitmap(bounds.Width, bounds.Height);

            // 使用Graphics类绘制控件内容到位图上
            graphics = Graphics.FromImage(bitmap);
            graphics.CopyFromScreen(new System.Drawing.Point(bounds.X, bounds.Y), System.Drawing.Point.Empty, bounds.Size);
            Bitmap scaledBitmap = new Bitmap(bitmap, new System.Drawing.Size(SCR_WIDTH, SCR_HEIGHT));

            // 将缩小后的图像转换为16位RGB565格式的字节数组
            byte[] byteArray = ConvertTo16BitByteArray(scaledBitmap);

            // 释放资源
            bitmap.Dispose();
            // 释放资源
            scaledBitmap.Dispose();
            return byteArray;
        }

        private void btn_screenshot_Click(object sender, RoutedEventArgs e)
        {

            // 获取frm_media控件的位置和大小
            System.Drawing.Rectangle bounds = frm_media.Bounds;

            // 创建一个与控件大小相同的位图
            Bitmap bitmap = new Bitmap(bounds.Width, bounds.Height);

            // 使用Graphics类绘制控件内容到位图上
            graphics = Graphics.FromImage(bitmap);
            graphics.CopyFromScreen(new System.Drawing.Point(bounds.X, bounds.Y), System.Drawing.Point.Empty, bounds.Size);

            Bitmap scaledBitmap = new Bitmap(bitmap, new System.Drawing.Size(SCR_WIDTH, SCR_HEIGHT));

            // 保存位图为图片文件
            scaledBitmap.Save(@"E:\Downloads\media_screenshot.png", System.Drawing.Imaging.ImageFormat.Png);

            // 释放资源
            scaledBitmap.Dispose();
            // 释放资源
            bitmap.Dispose();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            usbalive = false;
        }

        private void btn_time_Click(object sender, RoutedEventArgs e)
        {
            SetTimestamp();
            long timestamp = 0;
            GetTimestamp(ref timestamp);
        }

        int brightness = 0;
        bool brightnessupdate = true;

        private void btn_start_Click(object sender, RoutedEventArgs e)
        {
            if (usbalive)
                return;
            usbalive = true;
            SetDirection(0);
            SetTimestamp();
            brightnessupdate = true;
            brightness = (int)sld_blk.Value;
            Thread thread = new Thread(() =>
            {
                while (usbalive)
                {
                    Thread.Sleep(1000);
                    Console.WriteLine($"FPS:{fps},PICFPS:{picfps}"); fps = 0; picfps = 0;
                }
            })
            { IsBackground = true };
            thread.Start();
            Thread threadreceive = new Thread(() =>
            {
                var recvdata = new byte[1024];
                while (usbalive)
                {
                    var ret = NexLink.receive(recvdata, 1024);
                    if (ret > 0)
                        Console.Write($"Receive:{Encoding.UTF8.GetString(recvdata, 0, ret)}");
                }
            })
            { IsBackground = true };
            threadreceive.Start();
            Thread threadsend = new Thread(() =>
            {
                while (usbalive)
                {
                    if (brightnessupdate)
                    {
                        brightnessupdate = false;
                        Stopwatch stopwatch = new Stopwatch();

                        stopwatch.Start();
                        SetBrightness(new nex_brightness_des() { brightness = (ushort)brightness, damp = 80 });
                        stopwatch.Stop();
                        Console.WriteLine($"Total time: {stopwatch.Elapsed.TotalMilliseconds:0.000}ms");
                    }
                    //Thread.Sleep(35);
                    if (rawdata != null)
                        try
                        {
                            SendOnPic(rawdata);
                            fps++;
                        }
                        catch (Exception)
                        {
                            Thread.Sleep(1000);
                        }
                }
                SetBrightness(new nex_brightness_des() { brightness = (ushort)0, damp = 5000 });
                NexLink.close();
            })
            { IsBackground = true };
            threadsend.Start();

            DispatcherTimer dispatcher = new DispatcherTimer();
            dispatcher.Interval = TimeSpan.FromMilliseconds(5);
            dispatcher.Tick += Dispatcher_Tick;
            dispatcher.Start();
        }

        private void btn_init_Click(object sender, RoutedEventArgs e)
        {
            var ret = NexLink.scandevices();
            Console.WriteLine($"Num of devices:{ret}");
            if (ret == 0) return;
            ret = NexLink.initwithindex(ret - 1);
            Console.WriteLine($"Ret:{ret}");
        }

        private void btn_stop_Click(object sender, RoutedEventArgs e)
        {
            usbalive = false;
        }

        private void sld_blk_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            brightness = (int)e.NewValue;
            brightnessupdate = true;
        }
    }
}
