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

    public enum candle_id_flags : UInt32
    {
        CANDLE_ID_EXTENDED = 0x80000000,
        CANDLE_ID_RTR = 0x40000000,
        CANDLE_ID_ERR = 0x20000000
    }

    enum candle_mode_t : UInt32
    {
        CANDLE_MODE_NORMAL = 0x00,
        CANDLE_MODE_LISTEN_ONLY = 0x01,
        CANDLE_MODE_LOOP_BACK = 0x02,
        CANDLE_MODE_TRIPLE_SAMPLE = 0x04,
        CANDLE_MODE_ONE_SHOT = 0x08,
        CANDLE_MODE_HW_TIMESTAMP = 0x10,
    };

    enum candle_devmode_t
    {
        CANDLE_DEVMODE_RESET = 0,
        CANDLE_DEVMODE_START = 1
    };

    [StructLayout(LayoutKind.Sequential)]
    struct candle_bittiming_t
    {
        public uint prop_seg;
        public uint phase_seg1;
        public uint phase_seg2;
        public uint sjw;
        public uint brp;
    };

    [StructLayout(LayoutKind.Sequential)]
    struct candle_device_mode_t
    {
        public uint mode;
        public uint flags;
    };

    [StructLayout(LayoutKind.Sequential)]
    struct candle_frame_t
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

    enum CANDLE_BREQ : Byte
    {
        CANDLE_BREQ_HOST_FORMAT = 0,
        CANDLE_BREQ_BITTIMING,
        CANDLE_BREQ_MODE,
        CANDLE_BREQ_BERR,
        CANDLE_BREQ_BT_CONST,
        CANDLE_BREQ_DEVICE_CONFIG,
        CANDLE_TIMESTAMP_GET,
    };

    public class Frame
    {
        public UInt32 Identifier;
        public byte[] Data;
        public UInt32 Timestamp;
        public bool Extended;
        public bool RTR;
        public bool Error;

        public override string ToString()
        {
            var value = String.Format("ID : {0}, Data : {1}, Time : {2}us"
                , this.Identifier
                , BitConverter.ToString(this.Data)
                , this.Timestamp
            );

            if (this.Extended)
            {
                value += " EXT";
            }
            if (this.RTR)
            {
                value += " RTR";
            }
            if (this.Error)
            {
                value += " Error";
            }

            return value;
        }

        // From https://en.wikipedia.org/wiki/CAN_bus#Frames
        public int LengthOnBus
        {
            get
            {
                if (!this.Extended)
                {
                    return 1 + 11 + 1 + 2 + 4 + 8 + 15 + 1 + 2 + 7 + 3;
                }
                else
                {
                    return 1 + 11 + 1 + 1 + 18 + 1 + 2 + 4 + (this.Data.Length * 8) + 15 + 1 + 1 + 1 + 7;
                }
            }
        }
    }

    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            this.Topmost = true;
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

        public int SetBitrate(byte ch, uint bitrate)
        {
            candle_bittiming_t t;
            t.prop_seg = 1;
            t.sjw = 1;
            t.phase_seg1 = 12 - 1;
            t.phase_seg2 = 1;

            switch (bitrate)
            {
                case 10000:
                    t.brp = 300;
                    break;

                case 20000:
                    t.brp = 150;
                    break;

                case 50000:
                    t.brp = 60;
                    break;

                case 83333:
                    t.brp = 36;
                    break;

                case 100000:
                    t.brp = 30;
                    break;

                case 125000:
                    t.brp = 24;
                    break;

                case 250000:
                    t.brp = 12;
                    break;

                case 500000:
                    t.brp = 6;
                    break;

                case 800000:
                    t.brp = 4;
                    t.phase_seg1 = 12 - t.prop_seg;
                    t.phase_seg2 = 2;
                    break;

                case 1000000:
                    t.brp = 3;
                    break;

                default:
                    return -1;
            }

            return NexLink.control_set((byte)CANDLE_BREQ.CANDLE_BREQ_BITTIMING, ch, StructToBytes(t), (ushort)Marshal.SizeOf(t));
        }

        public int ChannelStart(byte ch)
        {
            uint flags = (uint)candle_mode_t.CANDLE_MODE_NORMAL;
            flags |= (uint)candle_mode_t.CANDLE_MODE_HW_TIMESTAMP;

            candle_device_mode_t dm;
            dm.mode = (uint)candle_devmode_t.CANDLE_DEVMODE_START;
            dm.flags = flags;
            return NexLink.control_set((byte)CANDLE_BREQ.CANDLE_BREQ_MODE, ch, StructToBytes(dm), (ushort)Marshal.SizeOf(dm));
        }

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
            var ret = NexLink.init();
            Console.WriteLine($"ret:{ret}");
            if (ret < 0) return;
            SetBitrate(0, 500000);
            ChannelStart(0);

            DispatcherTimer dispatcher = new DispatcherTimer();
            dispatcher.Interval =TimeSpan.FromMilliseconds(20);
            dispatcher.Tick += Dispatcher_Tick;
            dispatcher.Start();
        }
        private bool flushbusy = false;
        private void Dispatcher_Tick(object sender, EventArgs e)
        {
            // 捕获屏幕指定区域的图像
            var point = frm_media.PointToScreen(System.Drawing.Point.Empty);
            System.Drawing.Rectangle rectangle = new System.Drawing.Rectangle(point.X, point.Y, frm_media.Bounds.Width, frm_media.Bounds.Height);
            ThreadPool.QueueUserWorkItem(new WaitCallback((trectangle) =>
            {
                var data = CaptureScreenPart(rectangle);
                if (flushbusy) return;
                flushbusy = true;
                SendOnPic(data);
                flushbusy = false;
        }), rectangle);
        }

        public void SendOnPic(byte[] data)
        {
            var recvdata = new byte[64];
            for (int i = 0; i < SCR_WIDTH * 4; i++)
            {
                for (int p = 0; p < 64; p++)
                {
                    if (p % 2 == 0)
                        recvdata[p] = data[i * 64 + p + 1];
                    else
                        recvdata[p] = data[i * 64 + p - 1];
                }
                NexLink.transfer(recvdata, 64);
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
            Bitmap scaledBitmap = new Bitmap(SCR_WIDTH, SCR_HEIGHT);
            // 使用Graphics类绘制控件内容到位图上
            using (Graphics graphics = Graphics.FromImage(bitmap))
            {
                graphics.CopyFromScreen(new System.Drawing.Point(bounds.X, bounds.Y), System.Drawing.Point.Empty, bounds.Size);
            }
            using (Graphics scaledGraphics = Graphics.FromImage(scaledBitmap))
            {
                scaledGraphics.DrawImage(bitmap, 0, 0, scaledBitmap.Width, scaledBitmap.Height);
            }

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
            SendOnPic(new byte[128*160*2]);
            NexLink.close();
        }
    }
}
