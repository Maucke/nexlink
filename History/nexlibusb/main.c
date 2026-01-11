#if 0
#include "nexlibusb.h"
#include <stdio.h>

#define USB_VID         0x1D50          //USB的产商ID
#define USB_PID         0x606F          //USB的产品ID
int main() {
    usb_init();
    int deviceCount = usb_find_devices(USB_VID, USB_PID);
    if (deviceCount > 0) {
        for (int i = 0; i < deviceCount; i++) {
            UsbDevice_Info info;
            usb_get_info(i, &info);
            if (usb_open_device(i)) {
                fprintf(stderr, "Error opening device %d\n", i);
            }
            else {
                // 对每个打开的设备进行操作
                unsigned char data[] = { 0x01, 0x02, 0x03 };
                int transferred;
                usb_bulk_transfer(i, 0x02, data, sizeof(data), &transferred, 10);
                usb_close_device(i);
            }
        }
    }
    else {
        fprintf(stderr, "No devices found\n");
    }
    return 0;
}
#endif
//using System;
//using System.Runtime.InteropServices;
//
//class Program
//{
//    [DllImport("your_library_name.dll")]
//        public static extern void usb_init();
//
//    [DllImport("your_library_name.dll")]
//        public static extern IntPtr usb_find_device(int vid, int pid);
//
//    [DllImport("your_library_name.dll")]
//        public static extern int usb_open_device(IntPtr device);
//
//    [DllImport("your_library_name.dll")]
//        public static extern int usb_close_device(IntPtr device);
//
//    [DllImport("your_library_name.dll")]
//        public static extern int usb_write_control(IntPtr device, byte requestType, byte request, ushort value, ushort index, byte[] data, uint length);
//
//    [DllImport("your_library_name.dll")]
//        public static extern int usb_read_control(IntPtr device, byte requestType, byte request, ushort value, ushort index, byte[] data, uint length);
//
//    [DllImport("your_library_name.dll")]
//        public static extern int usb_bulk_transfer(IntPtr device, byte endpoint, byte[] data, int length, ref int transferred);
//
//    static void Main()
//    {
//        usb_init();
//        IntPtr devicePtr = usb_find_device(0x1234, 0x5678);
//        if (devicePtr != IntPtr.Zero)
//        {
//            if (usb_open_device(devicePtr) != 0)
//            {
//                Console.WriteLine("Error opening device");
//            }
//            else
//            {
//                byte[] data = { 0x01, 0x02, 0x03 };
//                int transferred = 0;
//                usb_bulk_transfer(devicePtr, 0x02, data, data.Length, ref transferred);
//            }
//            usb_close_device(devicePtr);
//        }
//        else
//        {
//            Console.WriteLine("Device not found");
//        }
//    }
//}