using Hexconverters;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using NexLink_NET;

namespace NexLink_Net
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

    public class NexLinkUser : NexLink
    {
        public byte[] StructToBytes(object odata, byte[] exdata)
        {
            int size = Marshal.SizeOf(odata);
            byte[] byteArray = new byte[size + exdata.Length];

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
            Array.Copy(exdata, 0, byteArray, size, exdata.Length);
            return byteArray;
        }

        public nex_screen_des Screendes { get; set; }
        public string Version { get; set; }

        public bool GetLogDes(ref nex_log_des log)
        {
            var rawdata = new byte[StructToBytes(log).Length];
            var ret = control_in((byte)NEX_BREQ.NEX_LOG_SIZE_GET, rawdata, (ushort)rawdata.Length);
            log = (nex_log_des)BytesToStruct(rawdata, typeof(nex_log_des));
            return ret >= 0;
        }

        public bool GetLogData(ref nex_log_data log)
        {
            var rawdata = new byte[StructToBytes(log).Length];
            var ret = control_in((byte)NEX_BREQ.NEX_LOG_GET, rawdata, (ushort)rawdata.Length);
            log = (nex_log_data)BytesToStruct(rawdata, typeof(nex_log_data));
            return ret >= 0;
        }

        public bool SetI2cData(nex_i2c_request i2cRequest, byte[] writeBytes)
        {
            var rawdata = StructToBytes(i2cRequest, writeBytes);
            return control_out((byte)NEX_BREQ.NEX_I2C, rawdata, (ushort)rawdata.Length) >= 0;
        }

        public bool I2cInit(uint baudRate)
        {
            nex_i2c_init i2cInit = new nex_i2c_init() { baudRate = baudRate, channel = 0};
            var rawdata = StructToBytes(i2cInit);
            return control_out((byte)NEX_BREQ.NEX_I2C_INIT, rawdata, (ushort)rawdata.Length) >= 0;
        }

        public bool SetUartData(nex_uart_request uartRequest)
        {
            var rawdata = StructToBytes(uartRequest);
            return control_out((byte)NEX_BREQ.NEX_UART_TX, rawdata, (ushort)rawdata.Length) >= 0;
        }


        public bool GetUartData(nex_uart_request uartRequest)
        {
            var rawdata = StructToBytes(uartRequest);
            return control_out((byte)NEX_BREQ.NEX_UART_RX, rawdata, (ushort)rawdata.Length) >= 0;
        }

        object locker = new object();

        public uint I2cWriteRead(nex_i2c_request i2cRequest, byte[] writeBytes, ref byte[] readBytes)
        {
            int outLen = -1;
            var rawBytes = new byte[64];
            int retryTimes = 10;
            lock (locker)
            {
                // Debug.WriteLine("I2cWriteRead In");
                var ret = SetI2cData(i2cRequest, writeBytes);
                //if (ret != bool.SUCCESS && (int)ret <= (int)bool.ERROR_NOT_ACCESSED)
                //{
                //    // Debug.WriteLine("I2cWriteRead Out");
                //    return I2CErrorParser.HAL_I2C_WRONG_USB;
                //}
                while (retryTimes > 0)
                {
                    ReceiveData(ref rawBytes, rawBytes.Length, ref outLen);
                    // Debug.WriteLine($"I2cWriteRead:{Hexstring.ToString(rawBytes, outLen)}");
                    if (outLen > 0)
                    {
                        var log = (nex_log_data)BytesToStruct(rawBytes, typeof(nex_log_data));
                        if (log.type == CommunicationProtocol.PROTOCOL_I2C)
                        {
                            if (log.iserr && log.len == 2)
                            {
                                UInt16 errorCode = log.data[0];
                                errorCode |= (ushort)(log.data[1] << 8);
                                // Debug.WriteLine("I2cWriteRead Out");
                                return errorCode;
                            }
                            else if (!log.iserr)
                            {
                                readBytes = new byte[log.len];
                                Array.Copy(log.data, 0, readBytes, 0, log.len);
                                // Debug.WriteLine("I2cWriteRead Out");
                                return I2CErrorParser.HAL_I2C_ERROR_NONE;
                            }
                            // Debug.WriteLine("I2cWriteRead Out");
                            return I2CErrorParser.HAL_I2C_ERROR_SIZE;
                        }
                        else
                        {
                            // Debug.WriteLine("I2cWriteRead Out");
                            return I2CErrorParser.HAL_I2C_WRONG_USB;
                        }
                    }
                    retryTimes--;
                }
                // Debug.WriteLine("I2cWriteRead Out");
                return I2CErrorParser.HAL_I2C_ERROR_TIMEOUT;
            }
        }

        public uint UartWriteRead(nex_uart_request uartRequest, ref List<byte[]> readBytesList)
        {
            lock (locker)
            {
                var ret = GetUartData(uartRequest);
                if (!ret)
                {
                    return I2CErrorParser.HAL_I2C_WRONG_USB;
                }
                int outLen = -1;
                var rawBytes = new byte[64];
                readBytesList = new List<byte[]>();
                int retryTimes = 10;
                bool isContinue = true;
                do
                {
                    ReceiveData(ref rawBytes, rawBytes.Length, ref outLen);
                    // Debug.WriteLine($"UartWriteRead:{Hexstring.ToString(rawBytes, outLen)}");
                    if (outLen > 0)
                    {
                        var log = (nex_log_data)BytesToStruct(rawBytes, typeof(nex_log_data));
                        if (log.type == CommunicationProtocol.PROTOCOL_UART)
                        {
                            if (log.iserr && log.len == 2)
                            {
                                UInt16 errorCode = log.data[0];
                                errorCode |= (ushort)(log.data[1] << 8);
                                // Debug.WriteLine("UartWriteRead Out");
                                return errorCode;
                            }
                            else if (!log.iserr)
                            {
                                isContinue = log.iscontinue;
                                var readBytes = new byte[log.len];
                                Array.Copy(log.data, 0, readBytes, 0, log.len);
                                readBytesList.Add(readBytes);
                                //Debug.WriteLine(Hexstring.ToString(readBytes));
                            }
                            else
                            {
                                // Debug.WriteLine("UartWriteRead Out");
                                return I2CErrorParser.HAL_I2C_ERROR_SIZE;
                            }
                        }
                        else
                        {
                            // Debug.WriteLine("UartWriteRead Out");
                            return I2CErrorParser.HAL_I2C_WRONG_USB;
                        }
                    }
                    else
                        retryTimes--;
                } while (isContinue && retryTimes > 0);
                // Debug.WriteLine("UartWriteRead Out");
                return I2CErrorParser.HAL_I2C_ERROR_NONE;
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
        public byte dataWriteLength;         // 数据长度
        public byte dataReadLength;         // 数据长度
        public ushort cycle;        		    // cycleÊ±¼ä£¨ºÁÃë£©
        public ushort timeout;            // 超时时间（毫秒）

        //[MarshalAs(UnmanagedType.ByValArray, SizeConst = 64-8)]
        //public byte[] dataWriteBuffer;
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
