using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Threading;
using System.Timers;
using System.Windows.Forms;

namespace NexLinkTester
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

    internal class Program
    {
        public static byte[] StructToBytes(object odata)
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

        public static object BytesToStruct(byte[] byteArray, Type type)
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

        public static int SetBitrate(byte ch, uint bitrate)
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

        public static int ChannelStart(byte ch)
        {
            uint flags = (uint)candle_mode_t.CANDLE_MODE_NORMAL;
            flags |= (uint)candle_mode_t.CANDLE_MODE_HW_TIMESTAMP;

            candle_device_mode_t dm;
            dm.mode = (uint)candle_devmode_t.CANDLE_DEVMODE_START;
            dm.flags = flags;
            return NexLink.control_set((byte)CANDLE_BREQ.CANDLE_BREQ_MODE, ch, StructToBytes(dm), (ushort)Marshal.SizeOf(dm));
        }

        public static int SendOnChannel(Frame frame, byte ch, bool blocking = false)
        {
            var nativeFrame = new candle_frame_t();
            nativeFrame.can_id = frame.Identifier;
            if (frame.Extended)
            {
                nativeFrame.can_id |= (UInt32)candle_id_flags.CANDLE_ID_EXTENDED;
            }
            if (frame.RTR)
            {
                nativeFrame.can_id |= (UInt32)candle_id_flags.CANDLE_ID_RTR;
            }
            if (frame.Error)
            {
                nativeFrame.can_id |= (UInt32)candle_id_flags.CANDLE_ID_ERR;
            }

            nativeFrame.data = new byte[8];
            nativeFrame.can_dlc = (byte)frame.Data.Length;
            Buffer.BlockCopy(frame.Data, 0, nativeFrame.data, 0, frame.Data.Length);

            var lengthOnBus = frame.LengthOnBus;

            nativeFrame.echo_id = 0;
            nativeFrame.channel = ch;

            return NexLink.transfer(StructToBytes(nativeFrame), (ushort)Marshal.SizeOf(nativeFrame));
        }

        public static int ReadOnChannel(Frame frame, byte ch, bool blocking = false)
        {
            byte[] data = new byte[Marshal.SizeOf(typeof(candle_frame_t))];
            var ret = NexLink.receive(data, (ushort)Marshal.SizeOf(typeof(candle_frame_t)));
            if (ret < (Marshal.SizeOf(typeof(candle_frame_t)) - 4))
                return -1;
            var nativeFrame = (candle_frame_t)BytesToStruct(data, typeof(candle_frame_t));

            var flags = (candle_id_flags)(nativeFrame.can_id);
            frame.Identifier = nativeFrame.can_id & ((1 << 29) - 1);
            frame.Extended = flags.HasFlag(candle_id_flags.CANDLE_ID_EXTENDED);
            frame.RTR = flags.HasFlag(candle_id_flags.CANDLE_ID_RTR);
            frame.Error = flags.HasFlag(candle_id_flags.CANDLE_ID_ERR);

            frame.Data = new byte[nativeFrame.can_dlc];
            Buffer.BlockCopy(nativeFrame.data, 0, frame.Data, 0, nativeFrame.can_dlc > 8 ? 8 : nativeFrame.can_dlc);

            frame.Timestamp = nativeFrame.timestamp_us;
            return ret;
        }

        static void Main1(string[] args)
        {
            var ret = NexLink.init();
            Console.WriteLine($"ret:{ret}");
            if (ret < 0) return;
            SetBitrate(0, 500000);
            ChannelStart(0);
            var tempdata = new byte[100];
            ret = NexLink.control_get(17, 1, tempdata, (ushort)tempdata.Length);
            var data = new byte[] { 0xEE, 0x66, 0x00, 0x15, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55 };
            var recvdata = new byte[256];

            Random rd=new Random();
            for (int j = 0; j < 10000; j++)
            {
                for (int i = 0; i < 160*4; i++)
                {
                    for (int p = 0; p < 64; p++)
                    {
                        //if (j % 2 != 0)
                        //    recvdata[p] = gImage_1[i * 64 + p];
                        //else
                        //    recvdata[p] = gImage_2[i * 64 + p];
                    }
                    NexLink.transfer(recvdata, 64);
                }
            }
            Console.ReadKey();
            NexLink.close();
            Console.WriteLine("Hello World!");
        }
        static void Main(string[] args)
        {
            var ret = NexLink.init();
            Console.WriteLine($"ret:{ret}");
            if (ret < 0) return;
            SetBitrate(0, 500000);
            ChannelStart(0);
            var tempdata = new byte[100];
            var fps = 0;
            ret = NexLink.control_get(17, 1, tempdata, (ushort)tempdata.Length);
            var data = new byte[] { 0xEE, 0x66, 0x00, 0x15, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55 };

            Thread thread1 = new Thread(() => {
                while(true)
                {
                    Console.WriteLine($"FPS:{fps}"); fps = 0;
                    Thread.Sleep(1000);
                }
            })
            { IsBackground = true };
            thread1.Start();
            // 初始化定时器
            Thread thread = new Thread(() => {
                while(true)
                {
                    Timer_Tick();
                    fps++;
                }
            }) { IsBackground = true};
            thread.Start();
            Random rd = new Random();

            Console.ReadKey();
            NexLink.close();
            Console.WriteLine("Hello World!");
        }
        public static byte[] CaptureScreenPart(int targetWidth, int targetHeight)
        {
            Rectangle captureArea = new Rectangle(400, 400, targetWidth, targetHeight); // 设置要捕获的区域
            Bitmap bitmap = new Bitmap(captureArea.Width, captureArea.Height);
            using (Graphics g = Graphics.FromImage(bitmap))
            {
                g.CopyFromScreen(captureArea.Location, Point.Empty, captureArea.Size);
            }

            // 将缩小后的图像转换为16位RGB565格式的字节数组
            byte[] byteArray = ImageConverter.ConvertTo16BitByteArray(bitmap);

            return byteArray;
        }

        public static byte[] CaptureFullScreenAndResize(int targetWidth, int targetHeight)
        {
            // 捕获全屏图像
            Bitmap fullScreenBitmap = CaptureFullScreen();

            // 缩放图像到指定大小
            Bitmap resizedBitmap = ResizeBitmap(fullScreenBitmap, targetWidth, targetHeight);

            // 将缩小后的图像转换为16位RGB565格式的字节数组
            byte[] byteArray = ImageConverter.ConvertTo16BitByteArray(resizedBitmap);

            return byteArray;
        }

        private static Bitmap CaptureFullScreen()
        {
            Rectangle screenBounds = Screen.PrimaryScreen.Bounds;
            Bitmap fullScreenBitmap = new Bitmap(screenBounds.Width, screenBounds.Height);

            using (Graphics g = Graphics.FromImage(fullScreenBitmap))
            {
                g.CopyFromScreen(screenBounds.Location, Point.Empty, screenBounds.Size);
            }

            return fullScreenBitmap;
        }

        private static Bitmap ResizeBitmap(Bitmap bitmap, int targetWidth, int targetHeight)
        {
            Bitmap resizedBitmap = new Bitmap(targetWidth, targetHeight);

            using (Graphics g = Graphics.FromImage(resizedBitmap))
            {
                g.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
                g.DrawImage(bitmap, 0, 0, targetWidth, targetHeight);
            }

            return resizedBitmap;
        }

        private static void Timer_Tick()
        {
            // 捕获屏幕指定区域的图像
            //var data = CaptureScreenPart(160, 128);
            var data = new byte[128*160*2];

            var recvdata = new byte[1024];
            for (int i = 0; i < 40 * 1; i++)
            {
                for (int p = 0; p < 1024; p++)
                {
                    if (p % 2 == 0)
                        recvdata[p] = (byte)(p & 0xFF);
                    else
                        recvdata[p] = (byte)(p & 0xFF);
                }
                NexLink.transfer(recvdata, 1024);
            }
        }
    }
        public class ImageConverter
        {
            public static byte[] ConvertTo16BitByteArray(Bitmap bitmap)
            {
                // 将图像转换为16位RGB565格式
                BitmapData bmpData = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, PixelFormat.Format16bppRgb565);
                int byteCount = bmpData.Stride * bitmap.Height;
                byte[] byteArray = new byte[byteCount];
                IntPtr ptr = bmpData.Scan0;

                // 将像素数据复制到字节数组中
                System.Runtime.InteropServices.Marshal.Copy(ptr, byteArray, 0, byteCount);

                bitmap.UnlockBits(bmpData);

                return byteArray;
            }
    }

}
