using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace NexLinkLib
{
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
        const int MAX_MANUFACTURER_LENGTH = 256;
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct UsbDevice_Info
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string manufacturer;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string product;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string serial_number;
        }

        [DllImport("nexlibusb.dll")]
        private static extern void usb_init();

        [DllImport("nexlibusb.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void usb_get_info(int index, ref UsbDevice_Info info);

        [DllImport("nexlibusb.dll")]
        private static extern int usb_find_devices(int vid, int pid);

        [DllImport("nexlibusb.dll")]
        private static extern int usb_open_device(int index);

        [DllImport("nexlibusb.dll")]
        private static extern int usb_close_device(int index);

        [DllImport("nexlibusb.dll")]
        private static extern int usb_write_control(int index, byte requestType, byte request, ushort value, byte[] data, uint length);

        [DllImport("nexlibusb.dll")]
        private static extern int usb_read_control(int index, byte requestType, byte request, ushort value, byte[] data, uint length);

        [DllImport("nexlibusb.dll")]
        private static extern int usb_bulk_transfer(int index, byte endpoint, byte[] data, int length, ref int transferred);

        const int USB_VID = 0x1D50;
        const int USB_PID = 0x606F;
        const int USB_RECIP_MASK = 0x1f;
        const int USB_RECIP_DEVICE = 0x00;
        const int USB_RECIP_INTERFACE = 0x01;
        const int USB_RECIP_ENDPOINT = 0x02;
        const int USB_RECIP_OTHER = 0x03;
        const int USB_TYPE_MASK = 0x03 << 5;
        const int USB_TYPE_STANDARD = 0x00 << 5;
        const int USB_TYPE_CLASS = 0x01 << 5;
        const int USB_TYPE_VENDOR = 0x02 << 5;
        const int USB_TYPE_RESERVED = 0x03 << 5;
        // 初始化 USB
        public static void Init()
        {
            usb_init();
        }

        // 查找设备
        public static int FindDevices()
        {
            return usb_find_devices(USB_VID, USB_PID);
        }

        public static UsbDevice_Info GetDeivceInfo(int index)
        {
            UsbDevice_Info info = new UsbDevice_Info();
            usb_get_info(index, ref info);

            return info;
        }

        public int currentIndex = -1;
        public bool isConnected = false;

        // 打开设备
        public bool OpenDevice(int index)
        {
            currentIndex = index;
            int result = usb_open_device(currentIndex);
            if (result == 0)
                isConnected = true;
            return result == 0;
        }

        // 关闭设备
        public void CloseDevice()
        {
            if (currentIndex != -1 && isConnected)
            {
                usb_close_device(currentIndex);
            }
            isConnected = false;
        }

        // 写控制指令
        public int WriteControl(byte request, ushort value, byte[] data, uint length)
        {
            if (currentIndex != -1 && isConnected)
            {
                return usb_write_control(currentIndex, USB_TYPE_VENDOR | USB_RECIP_INTERFACE, request, value, data, length);
            }
            else
            {
                return -1; // 可以根据实际情况返回一个错误码
            }
        }

        // 读控制指令
        public int ReadControl(byte request, ushort value, byte[] data, uint length)
        {
            if (currentIndex != -1 && isConnected)
            {
                return usb_read_control(currentIndex, USB_TYPE_VENDOR | USB_RECIP_INTERFACE, request, value, data, length);
            }
            else
            {
                return -1; // 可以根据实际情况返回一个错误码
            }
        }

        public LibUsbError TransferData(byte chn, byte[] data, int length, out int transferred)
        {
            transferred = 0;
            if (currentIndex != -1 && isConnected)
            {
                return (LibUsbError)usb_bulk_transfer(currentIndex, chn, data, length, ref transferred);
            }
            else
            {
                return (LibUsbError)(-1); // 可以根据实际情况返回一个错误码
            }
        }

        public LibUsbError ReceiverData(byte chn, byte[] data, int length, out int transferred)
        {
            transferred = 0;
            if (currentIndex != -1 && isConnected)
            {
                return (LibUsbError)usb_bulk_transfer(currentIndex, (byte)(chn | 0x80), data, length, ref transferred);
            }
            else
            {
                return (LibUsbError)(-1); // 可以根据实际情况返回一个错误码
            }
        }

#if false
        public static void Main()
        {
            NexLink nexLink = new NexLink();
            InitializeUsb();
            int deviceCount = FindDevices();
            if (deviceCount > 0)
            {
                for (int i = 0; i < deviceCount; i++)
                {
                    if (nexLink.OpenDevice(i))
                    {
                        var info = nexLink.GetDeivceInfo();
                        // 对每个打开的设备进行操作
                        byte[] data = { 0x01, 0x02, 0x03 };
                        int transferred;
                        nexLink.TransferData(data, data.Length, out transferred);
                        nexLink.CloseDevice();
                    }
                    else
                    {
                        Console.WriteLine($"Error opening device {i}");
                    }
                }
            }
            else
            {
                Console.WriteLine("No devices found");
            }
        }
#endif
    }
}
