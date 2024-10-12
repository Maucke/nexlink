using Hexconverters;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace NexLinker
{
    // 定义 C 结构体的映射
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct DeviceInfo
    {
        public ushort vendor_id;
        public ushort product_id;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string manufacturer;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string product;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string serial_number;
    }

    public enum LibUsbError
    {
        SUCCESS = 0,
        ERROR_IO = 1,
        ERROR_INVALID_PARAM = 2,
        ERROR_NO_DEVICE = 3,
        ERROR_NOT_SUPPORTED = 4,
        ERROR_TIMEOUT = 5,
        ERROR_OVERFLOW = 6,
        ERROR_PIPE = 7,
        ERROR_INTERRUPTED = 8,
        ERROR_NO_MEM = 9,
        ERROR_NOT_ACCESSED = 10
    }

    public class NexLink
    {
        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern int Init();

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int Scan();

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int Open(int index);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Close(int index);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int Receive(int index, byte[] data, int length, ref int length_actual);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int Transfer(int index, byte[] data, int length, ref int length_actual);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int ControlGet(int index, byte bRequest, ushort wValue, byte[] data, ushort wLength);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int ControlSet(int index, byte bRequest, ushort wValue, byte[] data, ushort wLength);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void GetDeviceInfo(int index, ref DeviceInfo info);

        public bool IsConnected = false;

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

        public nex_screen_des Screendes { get; set; }
        public string Version { get; set; }

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

        int Index = -1;

        public bool OpenDevice(int index)
        {
            Index = index;
            if (Open(index) > 0)
                IsConnected = true;
            else
                IsConnected = false;
            return IsConnected;
        }

        public int GetIndex()
        {
            return Index;
        }

        public void CloseDevice()
        {
            Close(Index);
            IsConnected = false;
            Index = -1;
        }

        public static List<DeviceInfo> ScanDevices()
        {
            List<DeviceInfo> infos = new List<DeviceInfo>();

            int count = Scan();

            for (int i = 0; i < count; i++)
            {
                DeviceInfo info = new DeviceInfo();
                GetDeviceInfo(i, ref info);
                infos.Add(info);
            }
            return infos;
        }

        public LibUsbError ReceiveData(ref byte[] data, int length, ref int length_actual)
        {
            return (LibUsbError)Receive(Index, data, length, ref length_actual);
        }

        public LibUsbError TransferData(byte[] data, int length, ref int length_actual)
        {
            return (LibUsbError)Transfer(Index, data, length, ref length_actual);
        }

        public LibUsbError ControlSetData(NEX_BREQ cmd, byte[] data)
        {
            return (LibUsbError)ControlSet(Index, (byte)cmd, 0, data, (ushort)data.Length);
        }

        public LibUsbError ControlSetData(NEX_BREQ cmd, object odata)
        {
            var data = StructToBytes(odata);
            return (LibUsbError)ControlSet(Index, (byte)cmd, 0, data, (ushort)data.Length);
        }

        public LibUsbError ControlGetData(NEX_BREQ cmd, byte[] data)
        {
            return (LibUsbError)ControlGet(Index, (byte)cmd, 0, data, (ushort)data.Length);
        }

        public LibUsbError ControlGetData(NEX_BREQ cmd, ref object odata, Type type)
        {
            var data = new byte[Marshal.SizeOf(type)];
            var ret = ControlGet(Index, (byte)cmd, 0, data, (ushort)data.Length);
            if (ret >= 0)
                odata = BytesToStruct(data, type);
            return (LibUsbError)ret;
        }

        public LibUsbError SetTimestamp()
        {
            DateTime currentTime = DateTime.Now;
            DateTime unixStartTime = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Local);
            TimeSpan elapsedTime = currentTime - unixStartTime;
            long timestamp = (long)elapsedTime.TotalSeconds;

            var rawdata = BitConverter.GetBytes(timestamp);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_TIMESTAMP_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public LibUsbError GetTimestamp(ref long timestamp)
        {
            var rawdata = new byte[BitConverter.GetBytes(timestamp).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_TIMESTAMP_GET, 0, rawdata, (ushort)rawdata.Length);
            timestamp = BitConverter.ToInt64(rawdata, 0);
            return (LibUsbError)ret;
        }

        public LibUsbError SetBrightness(nex_brightness_des brides)
        {
            var rawdata = StructToBytes(brides);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_BRIGHTNESS_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public LibUsbError GetBrightness(ref nex_brightness_des brides)
        {
            var rawdata = new byte[StructToBytes(brides).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_BRIGHTNESS_GET, 0, rawdata, (ushort)rawdata.Length);
            brides = (nex_brightness_des)BytesToStruct(rawdata, typeof(nex_brightness_des));
            return (LibUsbError)ret;
        }

        public LibUsbError SetScreenDes(nex_screen_des screen)
        {
            var rawdata = StructToBytes(screen);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_SCREEN_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public LibUsbError GetScreenDes(ref nex_screen_des screen)
        {
            var rawdata = new byte[StructToBytes(screen).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_SCREEN_GET, 0, rawdata, (ushort)rawdata.Length);
            screen = (nex_screen_des)BytesToStruct(rawdata, typeof(nex_screen_des));
            return (LibUsbError)ret;
        }

        public LibUsbError GetLogDes(ref nex_log_des log)
        {
            var rawdata = new byte[StructToBytes(log).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_LOG_SIZE_GET, 0, rawdata, (ushort)rawdata.Length);
            log = (nex_log_des)BytesToStruct(rawdata, typeof(nex_log_des));
            return (LibUsbError)ret;
        }

        public LibUsbError GetLogData(ref nex_log_data log)
        {
            var rawdata = new byte[StructToBytes(log).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_LOG_GET, 0, rawdata, (ushort)rawdata.Length);
            log = (nex_log_data)BytesToStruct(rawdata, typeof(nex_log_data));
            return (LibUsbError)ret;
        }

        public LibUsbError GetI2cData(ref nex_i2c_request i2c)
        {
            var rawdata = new byte[StructToBytes(i2c).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_I2C_GET, 0, rawdata, (ushort)rawdata.Length);
            i2c = (nex_i2c_request)BytesToStruct(rawdata, typeof(nex_i2c_request));
            return (LibUsbError)ret;
        }

        public LibUsbError SetI2cData(nex_i2c_request i2c)
        {
            var rawdata = StructToBytes(i2c);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_I2C_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public LibUsbError GetNameDes(ref string name)
        {
            var rawdata = new byte[128];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_NAME_GET, 0, rawdata, (ushort)rawdata.Length);
            name = Encoding.UTF8.GetString(rawdata);
            return (LibUsbError)ret;
        }

        public LibUsbError GetVerDes(ref string version)
        {
            var rawdata = new byte[128];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_VERSION_GET, 0, rawdata, (ushort)rawdata.Length);
            version = Encoding.UTF8.GetString(rawdata);
            return (LibUsbError)ret;
        }

        public byte[] ScreenGram { get; set; }

        public void TransferImageData(int width, int height, int blocksize, byte[] imageData)
        {
            int blockSize = blocksize;  // Assuming BLOK_VALID is a constant defined elsewhere
            int bytesPerBlock = 2;  // Assuming each block consists of 2 bytes
            int blocksPerIteration = (width * height * bytesPerBlock) / blockSize;
            int length_actual = 0;
            byte[] transferBuffer = new byte[blockSize];

            for (int i = 0; i < blocksPerIteration; i++)
            {
                for (int p = 0; p < blockSize; p++)
                {
                    if ((p & 1) == 0)
                    {
                        transferBuffer[p] = imageData[i * blockSize + p + 1];
                    }
                    else
                    {
                        transferBuffer[p] = imageData[i * blockSize + p - 1];
                    }
                }
                TransferData(transferBuffer, transferBuffer.Length, ref length_actual);
            }
        }
        public void TransferImageData(byte[] imageData)
        {
            if (imageData == null || imageData.Length < 2)
            {
                return;
            }

            int length_actual = 0;

            // 创建一个新的数组来保存互换后的数据
            byte[] swappedData = new byte[imageData.Length];

            // 遍历每个字节，进行奇偶交换
            for (int i = 0; i < imageData.Length; i += 2)
            {
                var temp = imageData[i];
                // 交换当前字节和下一个字节
                swappedData[i] = imageData[i + 1];
                swappedData[i + 1] = temp;
            }
            int bufferSize = 1000; // 每次发送的字节数
            for (int i = 0; i < swappedData.Length; i += bufferSize)
            {
                int bytesToSend = Math.Min(bufferSize, swappedData.Length - i);
                byte[] tempBuffer = new byte[bytesToSend];
                Array.Copy(swappedData, i, tempBuffer, 0, bytesToSend);
                TransferData(tempBuffer, bytesToSend, ref length_actual);
                // Debug.Write(Hexstring.ToString(tempBuffer)+" ");
            }
        }
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
        NEX_NAME_GET,
        NEX_VERSION_GET,
        NEX_LOG_GET = 0x10,
        NEX_LOG_SIZE_GET,
        NEX_I2C_SET = 0x20,
        NEX_I2C_GET,
        NEX_COMMAND_LEN,
    };

    public enum CommunicationProtocol : Byte
    {
        PROTOCOL_LOG,      // 提示信息通信
        PROTOCOL_UART = 1, // 串口通信
        PROTOCOL_I2C,      // I2C通信
        PROTOCOL_SPI,      // SPI通信
        PROTOCOL_CAN,      // CAN通信
        PROTOCOL_ETHERNET, // 以太网通信
        PROTOCOL_USB       // USB通信
    }

    public enum TxRxMode : Byte
    {
        Tx = 0,  // 发送
        Rx        // 接收
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_brightness_des
    {
        public UInt16 brightness;
        public UInt16 damp;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_screen_des
    {
        public UInt16 width;
        public UInt16 height;
        public UInt16 blocksize;
        public byte direction;
        public UInt16 startx;
        public UInt16 starty;
        public UInt16 picw;
        public UInt16 pich;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_log_des
    {
        public ushort size;
        public ushort maxSize;
        public ushort isFull;
        public ushort reserve;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_log_data
    {
        public uint timestamp;                       // 对应 unsigned int
        public byte len;                             // 对应 unsigned char
        public CommunicationProtocol type;           // 对应 CommunicationProtocol 枚举
        public TxRxMode dir;                         // 对应 TxRxMode 枚举
        public byte reserve2;                        // 对应 unsigned char

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 56)] // 64 - 8 = 56
        public byte[] data;                          // 对应 unsigned char data[64-8]
    }

    public struct nex_usb_des
    {
        public UInt64 timestamp_s;
        public nex_brightness_des brides;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_i2c_request
    {
        public ushort deviceAddress;      // I2C 从设备地址
        public ushort dataWriteLength;         // 数据长度
        public ushort dataReadLength;         // 数据长度
        public uint timeout;            // 超时时间（毫秒）

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
        public byte[] dataWriteBuffer;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
        public byte[] dataReadBuffer;
    }

}
