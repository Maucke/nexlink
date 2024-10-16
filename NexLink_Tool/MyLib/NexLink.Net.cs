using Hexconverters;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace NexLinker
{
    public class I2CErrorParser
    {
        // 定义错误代码
        public const uint HAL_I2C_ERROR_NONE = 0x00000000U;    // 无错误
        public const uint HAL_I2C_ERROR_BERR = 0x00000001U;    // BERR 错误
        public const uint HAL_I2C_ERROR_ARLO = 0x00000002U;    // ARLO 错误
        public const uint HAL_I2C_ERROR_AF = 0x00000004U;      // AF 错误
        public const uint HAL_I2C_ERROR_OVR = 0x00000008U;     // OVR 错误
        public const uint HAL_I2C_ERROR_DMA = 0x00000010U;     // DMA 传输错误
        public const uint HAL_I2C_ERROR_TIMEOUT = 0x00000020U; // 超时错误
        public const uint HAL_I2C_ERROR_SIZE = 0x00000040U;    // 尺寸管理错误
        public const uint HAL_I2C_ERROR_DMA_PARAM = 0x00000080U; // DMA 参数错误
        public const uint HAL_I2C_WRONG_START = 0x00000200U;   // 错误的开始错误
        public const uint HAL_I2C_WRONG_USB = 0x00000400U;   // 错误的USB相关错误

        public static string ParseI2CError(uint errorCode)
        {
            if (errorCode == HAL_I2C_ERROR_NONE)
                return "无错误";

            string errorMessage = "发生错误：";

            if ((errorCode & HAL_I2C_ERROR_BERR) != 0)
                errorMessage += "总线错误; ";
            if ((errorCode & HAL_I2C_ERROR_ARLO) != 0)
                errorMessage += "仲裁丢失; ";
            if ((errorCode & HAL_I2C_ERROR_AF) != 0)
                errorMessage += "无应答; ";
            if ((errorCode & HAL_I2C_ERROR_OVR) != 0)
                errorMessage += "溢出错误; ";
            if ((errorCode & HAL_I2C_ERROR_DMA) != 0)
                errorMessage += "DMA 传输错误; ";
            if ((errorCode & HAL_I2C_ERROR_TIMEOUT) != 0)
                errorMessage += "超时错误; ";
            if ((errorCode & HAL_I2C_ERROR_SIZE) != 0)
                errorMessage += "尺寸管理错误; ";
            if ((errorCode & HAL_I2C_ERROR_DMA_PARAM) != 0)
                errorMessage += "DMA 参数错误; ";
            if ((errorCode & HAL_I2C_WRONG_START) != 0)
                errorMessage += "错误的开始错误; ";
            if ((errorCode & HAL_I2C_WRONG_USB) != 0)
                errorMessage += "错误的USB相关错误; ";

            return errorMessage.TrimEnd(';', ' '); // 去掉最后的分号和空格
        }
    }

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
        private static extern int Receive(int index, byte[] data, int length, ref int length_actual, uint timeout);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int Transfer(int index, byte[] data, int length, ref int length_actual, uint timeout);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int ControlGet(int index, byte bRequest, ushort wValue, byte[] data, ushort wLength, uint timeout);

        [DllImport("NexLink.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern int ControlSet(int index, byte bRequest, ushort wValue, byte[] data, ushort wLength, uint timeout);

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

        public LibUsbError ReceiveData(ref byte[] data, int length, ref int length_actual, uint timeout = 100)
        {
            return (LibUsbError)Receive(Index, data, length, ref length_actual, timeout);
        }

        public LibUsbError TransferData(byte[] data, int length, ref int length_actual, uint timeout = 100)
        {
            return (LibUsbError)Transfer(Index, data, length, ref length_actual, timeout);
        }

        public LibUsbError ControlSetData(NEX_BREQ cmd, byte[] data)
        {
            return (LibUsbError)ControlSet(Index, (byte)cmd, 0, data, (ushort)data.Length, 100);
        }

        public LibUsbError ControlSetData(NEX_BREQ cmd, object odata)
        {
            var data = StructToBytes(odata);
            return (LibUsbError)ControlSet(Index, (byte)cmd, 0, data, (ushort)data.Length, 100);
        }

        public LibUsbError ControlGetData(NEX_BREQ cmd, byte[] data)
        {
            return (LibUsbError)ControlGet(Index, (byte)cmd, 0, data, (ushort)data.Length, 100);
        }

        public LibUsbError ControlGetData(NEX_BREQ cmd, ref object odata, Type type)
        {
            var data = new byte[Marshal.SizeOf(type)];
            var ret = ControlGet(Index, (byte)cmd, 0, data, (ushort)data.Length, 100);
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
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_TIMESTAMP_SET, 0, rawdata, (ushort)rawdata.Length, 100);
        }

        public LibUsbError GetTimestamp(ref long timestamp)
        {
            var rawdata = new byte[BitConverter.GetBytes(timestamp).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_TIMESTAMP_GET, 0, rawdata, (ushort)rawdata.Length, 100);
            timestamp = BitConverter.ToInt64(rawdata, 0);
            return (LibUsbError)ret;
        }

        public LibUsbError SetBrightness(nex_brightness_des brides)
        {
            var rawdata = StructToBytes(brides);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_BRIGHTNESS_SET, 0, rawdata, (ushort)rawdata.Length, 100);
        }

        public LibUsbError GetBrightness(ref nex_brightness_des brides)
        {
            var rawdata = new byte[StructToBytes(brides).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_BRIGHTNESS_GET, 0, rawdata, (ushort)rawdata.Length, 100);
            brides = (nex_brightness_des)BytesToStruct(rawdata, typeof(nex_brightness_des));
            return (LibUsbError)ret;
        }

        public LibUsbError SetScreenDes(nex_screen_des screen)
        {
            var rawdata = StructToBytes(screen);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_SCREEN_SET, 0, rawdata, (ushort)rawdata.Length, 100);
        }

        public LibUsbError GetScreenDes(ref nex_screen_des screen)
        {
            var rawdata = new byte[StructToBytes(screen).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_SCREEN_GET, 0, rawdata, (ushort)rawdata.Length, 100);
            screen = (nex_screen_des)BytesToStruct(rawdata, typeof(nex_screen_des));
            return (LibUsbError)ret;
        }

        public LibUsbError GetLogDes(ref nex_log_des log)
        {
            var rawdata = new byte[StructToBytes(log).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_LOG_SIZE_GET, 0, rawdata, (ushort)rawdata.Length, 100);
            log = (nex_log_des)BytesToStruct(rawdata, typeof(nex_log_des));
            return (LibUsbError)ret;
        }

        public LibUsbError GetLogData(ref nex_log_data log)
        {
            var rawdata = new byte[StructToBytes(log).Length];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_LOG_GET, 0, rawdata, (ushort)rawdata.Length, 100);
            log = (nex_log_data)BytesToStruct(rawdata, typeof(nex_log_data));
            return (LibUsbError)ret;
        }

        public LibUsbError SetI2cData(nex_i2c_request i2cRequest)
        {
            var rawdata = StructToBytes(i2cRequest);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_I2C, 0, rawdata, (ushort)rawdata.Length, 100);
        }

        public LibUsbError I2cInit(uint baudRate)
        {
            nex_i2c_init i2cInit = new nex_i2c_init() { baudRate = baudRate, channel = 0};
            var rawdata = StructToBytes(i2cInit);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_I2C_INIT, 0, rawdata, (ushort)rawdata.Length, 100);
        }

        public LibUsbError SetUartData(nex_uart_request uartRequest)
        {
            var rawdata = StructToBytes(uartRequest);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_UART_TX, 0, rawdata, (ushort)rawdata.Length, 100);
        }


        public LibUsbError GetUartData(nex_uart_request uartRequest)
        {
            var rawdata = StructToBytes(uartRequest);
            return (LibUsbError)NexLink.ControlSet(Index, (byte)NEX_BREQ.NEX_UART_RX, 0, rawdata, (ushort)rawdata.Length, 100);
        }

        public LibUsbError GetNameDes(ref string name)
        {
            var rawdata = new byte[128];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_NAME_GET, 0, rawdata, (ushort)rawdata.Length, 100);
            name = Encoding.UTF8.GetString(rawdata);
            return (LibUsbError)ret;
        }

        public LibUsbError GetVerDes(ref string version)
        {
            var rawdata = new byte[128];
            var ret = NexLink.ControlGet(Index, (byte)NEX_BREQ.NEX_VERSION_GET, 0, rawdata, (ushort)rawdata.Length, 100);
            version = Encoding.UTF8.GetString(rawdata);
            return (LibUsbError)ret;
        }

        object locker = new object();

        public uint I2cWriteRead(nex_i2c_request i2cRequest, ref byte[] readBytes)
        {
            lock (locker)
            {
                var ret = SetI2cData(i2cRequest);
                if (ret != LibUsbError.SUCCESS && (int)ret <= (int)LibUsbError.ERROR_NOT_ACCESSED)
                    return I2CErrorParser.HAL_I2C_WRONG_USB;
                int outLen = -1;
                var rawBytes = new byte[64];
                ReceiveData(ref rawBytes, rawBytes.Length, ref outLen, (uint)(i2cRequest.timeout * 2));
                if (outLen > 0)
                {
                    var log = (nex_log_data)BytesToStruct(rawBytes, typeof(nex_log_data));
                    if (log.type == CommunicationProtocol.PROTOCOL_I2C)
                    {
                        if (log.iserr && log.len == 2)
                        {
                            UInt16 errorCode = log.data[0];
                            errorCode |= (ushort)(log.data[1] << 8);
                            return errorCode;
                        }
                        else if (!log.iserr)
                        {
                            readBytes = new byte[log.len];
                            Array.Copy(log.data, 0, readBytes, 0, log.len);
                            return I2CErrorParser.HAL_I2C_ERROR_NONE;
                        }
                        return I2CErrorParser.HAL_I2C_ERROR_SIZE;
                    }
                    else return I2CErrorParser.HAL_I2C_WRONG_USB;
                }
                return I2CErrorParser.HAL_I2C_ERROR_TIMEOUT;
            }
        }

        public uint UartWriteRead(nex_uart_request uartRequest, ref List<byte[]> readBytesList)
        {
            lock (locker)
            {
                var ret = GetUartData(uartRequest);
                if (ret != LibUsbError.SUCCESS && (int)ret <= (int)LibUsbError.ERROR_NOT_ACCESSED)
                    return I2CErrorParser.HAL_I2C_WRONG_USB;
                int outLen = -1;
                var rawBytes = new byte[64];
                readBytesList = new List<byte[]>();
                bool iscontinue;
                do
                {
                    ReceiveData(ref rawBytes, rawBytes.Length, ref outLen);
                    // Debug.WriteLine($"ReceiveData:{Hexstring.ToString(rawBytes)},{outLen}");
                    if (outLen > 0)
                    {
                        var log = (nex_log_data)BytesToStruct(rawBytes, typeof(nex_log_data));
                        if (log.type == CommunicationProtocol.PROTOCOL_UART)
                        {
                            if (log.iserr && log.len == 2)
                            {
                                UInt16 errorCode = log.data[0];
                                errorCode |= (ushort)(log.data[1] << 8);
                                return errorCode;
                            }
                            else if (!log.iserr)
                            {
                                iscontinue = log.iscontinue;
                                var readBytes = new byte[log.len];
                                Array.Copy(log.data, 0, readBytes, 0, log.len);
                                readBytesList.Add(readBytes);
                                //Debug.WriteLine(Hexstring.ToString(readBytes));
                            }
                            else
                                return I2CErrorParser.HAL_I2C_ERROR_SIZE;
                        }
                        else return I2CErrorParser.HAL_I2C_WRONG_USB;
                    }
                    else
                        iscontinue = false;
                } while (iscontinue);
                return I2CErrorParser.HAL_I2C_ERROR_NONE;
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
        NEX_I2C_INIT = 0x20,
        NEX_I2C,
        NEX_UART_INIT = 0x28,
        NEX_UART_TX,
        NEX_UART_RX,
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
        public byte properties;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 56)] // 64 - 8 = 56
        public byte[] data;                          // 对应 unsigned char data[64-8]

        public bool iserr
        {
            get { return (properties & 0x1) != 0; }
            set { properties = (byte)((properties & ~(0x1)) | (value ? 1 : 0)); }
        }

        public bool iscontinue
        {
            get { return (properties & 0x2) != 0; }
            set { properties = (byte)((properties & ~(0x2)) | (value ? 2 : 0)); }
        }
    }

    public struct nex_usb_des
    {
        public UInt64 timestamp_s;
        public nex_brightness_des brides;
    };

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_i2c_init
    {
        public byte channel;                // Channel
        public byte reserved1;      
        public ushort reserved2;         
        public uint baudRate;       
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct nex_i2c_request
    {
        public byte channel;                // Channel
        public byte deviceAddress;      // I2C 从设备地址
        public ushort dataWriteLength;         // 数据长度
        public ushort dataReadLength;         // 数据长度
        public ushort timeout;            // 超时时间（毫秒）

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64-8)]
        public byte[] dataWriteBuffer;
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
        //public byte[] dataReadBuffer;
    }


    [StructLayout(LayoutKind.Sequential)]
    public struct nex_uart_request
    {
        public byte channel;              
        public byte direction;     
        public ushort dataWriteLength;
        public uint reserved2;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64 - 8)]
        public byte[] dataWriteBuffer;
        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 256)]
        //public byte[] dataReadBuffer;
    }
}
