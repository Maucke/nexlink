using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace NexLinker
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

        public static int SetTimestamp()
        {
            DateTime currentTime = DateTime.Now;
            DateTime unixStartTime = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Local);
            TimeSpan elapsedTime = currentTime - unixStartTime;
            long timestamp = (long)elapsedTime.TotalSeconds;

            var rawdata = BitConverter.GetBytes(timestamp);
            return NexLink.control_set((byte)NEX_BREQ.NEX_TIMESTAMP_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public static int GetTimestamp(ref long timestamp)
        {
            var rawdata = new byte[BitConverter.GetBytes(timestamp).Length];
            var ret = NexLink.control_get((byte)NEX_BREQ.NEX_TIMESTAMP_GET, 0, rawdata, (ushort)rawdata.Length);
            timestamp = BitConverter.ToInt64(rawdata, 0);
            return ret;
        }

        public static int SetBrightness(nex_brightness_des brides)
        {
            var rawdata = StructToBytes(brides);
            return NexLink.control_set((byte)NEX_BREQ.NEX_BRIGHTNESS_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public static int GetBrightness(ref nex_brightness_des brides)
        {
            var rawdata = new byte[StructToBytes(brides).Length];
            var ret = NexLink.control_get((byte)NEX_BREQ.NEX_BRIGHTNESS_GET, 0, rawdata, (ushort)rawdata.Length);
            brides = (nex_brightness_des)BytesToStruct(rawdata, typeof(nex_brightness_des));
            return ret;
        }

        public static int SetScreenDes(nex_screen_des screen)
        {
            var rawdata = StructToBytes(screen);
            return NexLink.control_set((byte)NEX_BREQ.NEX_SCREEN_SET, 0, rawdata, (ushort)rawdata.Length);
        }

        public static int GetScreenDes(ref nex_screen_des screen)
        {
            var rawdata = new byte[StructToBytes(screen).Length];
            var ret = NexLink.control_get((byte)NEX_BREQ.NEX_SCREEN_GET, 0, rawdata, (ushort)rawdata.Length);
            screen = (nex_screen_des)BytesToStruct(rawdata, typeof(nex_screen_des));
            return ret;
        }

        public static int GetNameDes(ref string name)
        {
            var rawdata = new byte[128];
            var ret = NexLink.control_get((byte)NEX_BREQ.NEX_NAME_GET, 0, rawdata, (ushort)rawdata.Length);
            name = Encoding.UTF8.GetString(rawdata);
            return ret;
        }

        public static int GetVerDes(ref string version)
        {
            var rawdata = new byte[128];
            var ret = NexLink.control_get((byte)NEX_BREQ.NEX_VERSION_GET, 0, rawdata, (ushort)rawdata.Length);
            version = Encoding.UTF8.GetString(rawdata);
            return ret;
        }

        public static byte[] ScreenGram { get; set; }

        public static void TransferImageData(int width,int height,int blocksize,byte[] imageData)
        {
            int blockSize = blocksize;  // Assuming BLOK_VALID is a constant defined elsewhere
            int bytesPerBlock = 2;  // Assuming each block consists of 2 bytes
            int blocksPerIteration = (width * height * bytesPerBlock) / blockSize;

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

                NexLink.transfer(transferBuffer, transferBuffer.Length);
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
    };

    public struct nex_usb_des
    {
        public UInt64 timestamp_s;
        public nex_brightness_des brides;
    };
}
