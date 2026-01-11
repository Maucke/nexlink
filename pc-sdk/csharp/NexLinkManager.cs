using System;
using System.Collections.Generic;
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
    }
}
