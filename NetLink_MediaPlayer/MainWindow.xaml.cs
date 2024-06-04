using System;
using System.Collections.Generic;
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

    public enum NEX_id_flags : UInt32
    {
        NEX_ID_EXTENDED = 0x80000000,
        NEX_ID_RTR = 0x40000000,
        NEX_ID_ERR = 0x20000000
    }

    enum NEX_mode_t : UInt32
    {
        NEX_MODE_NORMAL = 0x00,
        NEX_MODE_LISTEN_ONLY = 0x01,
        NEX_MODE_LOOP_BACK = 0x02,
        NEX_MODE_TRIPLE_SAMPLE = 0x04,
        NEX_MODE_ONE_SHOT = 0x08,
        NEX_MODE_HW_TIMESTAMP = 0x10,
    };

    enum NEX_devmode_t
    {
        NEX_DEVMODE_RESET = 0,
        NEX_DEVMODE_START = 1
    };

    [StructLayout(LayoutKind.Sequential)]
    struct NEX_bittiming_t
    {
        public uint prop_seg;
        public uint phase_seg1;
        public uint phase_seg2;
        public uint sjw;
        public uint brp;
    };

    [StructLayout(LayoutKind.Sequential)]
    struct NEX_device_mode_t
    {
        public uint mode;
        public uint flags;
    };

    [StructLayout(LayoutKind.Sequential)]
    struct NEX_frame_t
    {
        public uint echo_id;
        public uint can_id;
        public byte can_dlc;
        public byte channel;
        public byte flags;
        public byte reserved;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public byte[] data;
        public uint timestamp_us;
    };

    enum NEX_BREQ : Byte
    {
        NEX_BREQ_HOST_FORMAT = 0,
        NEX_TIMESTAMP_SET,
        NEX_TIMESTAMP_GET,
        NEX_COMMAND_LEN,
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

        const int SCR_WIDTH = 160;
        const int SCR_HEIGHT = 128;

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

        int fps = 0;
        int picfps = 0;
        byte[] rawdata;
        bool usbalive = true;
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            AxWMPLib.AxWindowsMediaPlayer player = frm_media;
            player.URL = @"E:\Downloads\test.mp3"; // 替换为你想要播放的音频文件路径
            player.settings.enableErrorDialogs = true;
            player.settings.autoStart = false;
            player.settings.volume = 50;

            // 设置可视化效果类型
            player.settings.setMode("autoSize", true);
            player.settings.setMode("loop", false);
            player.settings.setMode("shuffle", false);

            // 显示可视化效果
            player.uiMode = "none";
            player.stretchToFit = true;
            var ret = NexLink.scandevices();
            Console.WriteLine($"Num of devices:{ret}");
            if (ret == 0) return;
            ret = NexLink.initwithindex(ret-1);
            Console.WriteLine($"Ret:{ret}");
        }
        private void Dispatcher_Tick(object sender, EventArgs e)
        {
            // 捕获屏幕指定区域的图像
            var point = frm_media.PointToScreen(System.Drawing.Point.Empty);
            System.Drawing.Rectangle rectangle = new System.Drawing.Rectangle(point.X, point.Y, frm_media.Bounds.Width, frm_media.Bounds.Height);
            var data = CaptureScreenPart(rectangle);
            rawdata = data;
            picfps++;
        }

        public void SendOnPic(byte[] data)
        {
            var recvdata = new byte[1024];
            for (int i = 0; i < (SCR_WIDTH * SCR_HEIGHT * 2) / 1024; i++)
            {
                for (int p = 0; p < 1024; p++)
                {
                    if (p % 2 == 0)
                        recvdata[p] = data[i * 1024 + p + 1];
                    else
                        recvdata[p] = data[i * 1024 + p - 1];
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

        public byte[] CaptureScreenPart(System.Drawing.Rectangle bounds)
        {
            // 创建一个与控件大小相同的位图
            Bitmap bitmap = new Bitmap(bounds.Width, bounds.Height);
            // 使用Graphics类绘制控件内容到位图上
            using (Graphics graphics = Graphics.FromImage(bitmap))
            {
                graphics.CopyFromScreen(new System.Drawing.Point(bounds.X, bounds.Y), System.Drawing.Point.Empty, bounds.Size);
            }
            Bitmap scaledBitmap = new Bitmap(bitmap, new System.Drawing.Size(SCR_WIDTH, SCR_HEIGHT));

            // 将缩小后的图像转换为16位RGB565格式的字节数组
            byte[] byteArray = ConvertTo16BitByteArray(scaledBitmap);

            return byteArray;
        }

        private void btn_screenshot_Click(object sender, RoutedEventArgs e)
        {

            // 获取frm_media控件的位置和大小
            System.Drawing.Rectangle bounds = frm_media.Bounds;

            // 创建一个与控件大小相同的位图
            Bitmap bitmap = new Bitmap(bounds.Width, bounds.Height);

            // 使用Graphics类绘制控件内容到位图上
            using (Graphics graphics = Graphics.FromImage(bitmap))
            {
                graphics.CopyFromScreen(frm_media.PointToScreen(System.Drawing.Point.Empty), System.Drawing.Point.Empty, bounds.Size);
            }

            // 保存位图为图片文件
            bitmap.Save(@"E:\Downloads\media_screenshot.png", System.Drawing.Imaging.ImageFormat.Png);

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

        private void btn_start_Click(object sender, RoutedEventArgs e)
        {
            SetTimestamp();
            Thread thread = new Thread(() =>
            {
                while (usbalive)
                {
                    Console.WriteLine($"FPS:{fps},PICFPS:{picfps}"); fps = 0; picfps = 0;
                    Thread.Sleep(1000);
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
                     Console.WriteLine($"Receive:{ret}"); 
                    Thread.Sleep(10);
                }
            })
            { IsBackground = true };
            threadreceive.Start();
            Thread threadsend = new Thread(() =>
            {
                while (usbalive)
                {
                    if (rawdata != null)
                    {
                        SendOnPic(rawdata);
                        fps++;
                    }
                }
                SendOnPic(new byte[SCR_WIDTH * SCR_HEIGHT * 2]);
                NexLink.close();
            })
            { IsBackground = true };
            threadsend.Start();

            DispatcherTimer dispatcher = new DispatcherTimer();
            dispatcher.Interval = TimeSpan.FromMilliseconds(5);
            dispatcher.Tick += Dispatcher_Tick;
            dispatcher.Start();
        }
    }
}
