using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace NexLink_NET
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct NexlinkDeviceInfo
    {
        public byte BusNumber;
        public byte DeviceAddress;
        public ushort VendorId;
        public ushort ProductId;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string Manufacturer;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string Product;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string Serial;

        public override string ToString()
        {
            return $"{Manufacturer} - {Product} (Bus:{BusNumber}, Addr:{DeviceAddress})";
        }

        // 获取显示名称
        public string GetDisplayName()
        {
            if (!string.IsNullOrEmpty(Manufacturer) && Manufacturer != "Unknown Manufacturer")
            {
                if (!string.IsNullOrEmpty(Product) && Product != "Unknown Product")
                {
                    return $"{Manufacturer} - {Product}";
                }
                return Manufacturer;
            }
            else if (!string.IsNullOrEmpty(Product) && Product != "Unknown Product")
            {
                return Product;
            }
            return $"Device {VendorId:X4}:{ProductId:X4}";
        }
    }

    public class NexLink : IDisposable
    {
        private const string DLL_NAME = "nexlibusb.dll"; // Windows

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int nexlink_init();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern void nexlink_cleanup();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern int nexlink_scan_devices(
            [In, Out] NexlinkDeviceInfo[] deviceList,
            int maxDevices);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern int nexlink_connect_device(byte busNumber, byte deviceAddress);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        private static extern int nexlink_configure_device();

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern int control_in(byte request, byte[] data, UInt16 size);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern int control_out(byte request, byte[] data, UInt16 size);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern int data_in(byte[] data, UInt16 size);

        [DllImport(DLL_NAME, CallingConvention = CallingConvention.Cdecl)]
        public static extern int data_out(byte[] data, UInt16 size);

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

        public bool IsConnected { get; private set; }

        private bool _disposed = false;

        public NexLink()
        {
            int result = nexlink_init();
            if (result < 0)
            {
                throw new InvalidOperationException($"Failed to initialize USB library: {result}");
            }
        }

        public static List<NexlinkDeviceInfo> ScanDevices(int maxDevices = 10)
        {
            var devices = new NexlinkDeviceInfo[maxDevices];
            int count = nexlink_scan_devices(devices, maxDevices);

            var result = new List<NexlinkDeviceInfo>();
            if (count > 0)
            {
                for (int i = 0; i < count; i++)
                {
                    result.Add(devices[i]);
                }
            }
            else if (count < 0)
            {
                throw new InvalidOperationException($"扫描设备失败，错误码: {count}");
            }

            return result;
        }

        public bool Connect(NexlinkDeviceInfo device)
        {
            // Open device
            int result = nexlink_connect_device(device.BusNumber, device.DeviceAddress);
            if (result < 0)
            {
                return false;
            }

            // Configure device
            result = nexlink_configure_device();
            if (result < 0)
            {
                return false;
            }

            IsConnected = true;

            return true;
        }

        public void Disconnect()
        {
            if (IsConnected)
            {
                IsConnected = false;
            }
        }
        #region IDisposable Implementation

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (!_disposed)
            {
                if (disposing)
                {
                    Disconnect();
                }

                nexlink_cleanup();
                _disposed = true;
            }
        }

        ~NexLink()
        {
            Dispose(false);
        }

        #endregion

        public bool SetTimestamp()
        {
            DateTime currentTime = DateTime.Now;
            DateTime unixStartTime = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Local);
            TimeSpan elapsedTime = currentTime - unixStartTime;
            long timestamp = (long)elapsedTime.TotalSeconds;

            var rawdata = BitConverter.GetBytes(timestamp);
            return control_out((byte)NEX_BREQ.NEX_TIMESTAMP_SET, rawdata, (ushort)rawdata.Length) >= 0;
        }

        public bool GetTimestamp(ref long timestamp)
        {
            var rawdata = new byte[BitConverter.GetBytes(timestamp).Length];
            var ret = control_in((byte)NEX_BREQ.NEX_TIMESTAMP_GET, rawdata, (ushort)rawdata.Length);
            timestamp = BitConverter.ToInt64(rawdata, 0);
            return ret >= 0;
        }

        public bool SetBrightness(nex_brightness_des brides)
        {
            var rawdata = StructToBytes(brides);
            return control_out((byte)NEX_BREQ.NEX_BRIGHTNESS_SET, rawdata, (ushort)rawdata.Length) >= 0;
        }

        public bool GetBrightness(ref nex_brightness_des brides)
        {
            var rawdata = new byte[StructToBytes(brides).Length];
            var ret = control_in((byte)NEX_BREQ.NEX_BRIGHTNESS_GET, rawdata, (ushort)rawdata.Length);
            brides = (nex_brightness_des)BytesToStruct(rawdata, typeof(nex_brightness_des));
            return ret >= 0;
        }

        public bool SetScreenDes(nex_screen_des screen)
        {
            var rawdata = StructToBytes(screen);
            return control_out((byte)NEX_BREQ.NEX_SCREEN_SET, rawdata, (ushort)rawdata.Length) >= 0;
        }

        public bool GetScreenDes(ref nex_screen_des screen)
        {
            var rawdata = new byte[StructToBytes(screen).Length];
            var ret = control_in((byte)NEX_BREQ.NEX_SCREEN_GET, rawdata, (ushort)rawdata.Length);
            screen = (nex_screen_des)BytesToStruct(rawdata, typeof(nex_screen_des));
            return ret >= 0;
        }

        public bool GetNameDes(ref string name)
        {
            var rawdata = new byte[128];
            var ret = control_in((byte)NEX_BREQ.NEX_NAME_GET, rawdata, (ushort)rawdata.Length);
            name = Encoding.UTF8.GetString(rawdata);
            return ret >= 0;
        }

        public bool GetVerDes(ref string version)
        {
            var rawdata = new byte[128];
            var ret = control_in((byte)NEX_BREQ.NEX_VERSION_GET, rawdata, (ushort)rawdata.Length);
            version = Encoding.UTF8.GetString(rawdata);
            return ret >= 0;
        }

        public byte[] ScreenGram { get; set; }

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
                UInt16 bytesToSend = (ushort)Math.Min(bufferSize, swappedData.Length - i);
                byte[] tempBuffer = new byte[bytesToSend];
                Array.Copy(swappedData, i, tempBuffer, 0, bytesToSend);
                length_actual = data_out(tempBuffer, bytesToSend);
                // Debug.Write(Hexstring.ToString(tempBuffer)+" ");
            }
        }

        public void ReceiveData(ref byte[] recvdata, int length, ref int count)
        {
            count = data_in(recvdata, (ushort)length);
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
        NEX_COMMAND_LEN,
    };

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

    public struct nex_usb_des
    {
        public UInt64 timestamp_s;
        public nex_brightness_des brides;
    };
}
