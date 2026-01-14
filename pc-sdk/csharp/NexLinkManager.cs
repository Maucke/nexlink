using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace NexLink
{
    public static class NexLinkManager
    {
        private const int SERIAL_LEN = 64;

        public static IReadOnlyList<string> Scan(int maxDevices = 16)
        {
            byte[] buf = new byte[maxDevices * SERIAL_LEN];
            int count = NexLinkNative.nexlink_scan(buf, maxDevices);

            List<string> result = new List<string>();
            for (int i = 0; i < count; i++)
            {
                string s = Encoding.ASCII.GetString(
                    buf, i * SERIAL_LEN, SERIAL_LEN).TrimEnd('\0');
                result.Add(s);
            }
            return result;
        }

        public static NexLinkDevice Open(string serial)
        {
            if (NexLinkNative.nexlink_open(serial, out var h) != 0)
                throw new InvalidOperationException("open failed");

            return new NexLinkDevice(h, serial);
        }
        public static byte[] StructToBytes<T>(T value) where T : struct
        {
            int size = Marshal.SizeOf<T>();
            byte[] buffer = new byte[size];

            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.StructureToPtr(value, ptr, false);
                Marshal.Copy(ptr, buffer, 0, size);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }

            return buffer;
        }

        public static T BytesToStruct<T>(byte[] data)
        where T : struct
        {
            int size = Marshal.SizeOf<T>();
            if (data.Length < size)
                throw new ArgumentException("Payload too small");

            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.Copy(data, 0, ptr, size);
                return Marshal.PtrToStructure<T>(ptr);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }
        public static ReadOnlySpan<byte> GetRespData(NexLinkPacket resp)
        {
            if (resp.payload == null || resp.payload.Length <= 1)
                throw new InvalidOperationException("Response has no data");

            return resp.payload.AsSpan(1);
        }
    }
}
