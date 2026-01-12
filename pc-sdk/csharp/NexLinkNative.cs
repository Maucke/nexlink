using System;
using System.Runtime.InteropServices;

namespace NexLink
{
    internal static class NexLinkNative
    {
        private const string DLL = "nexlink_usb.dll";

        /* ===== scan ===== */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int nexlink_scan(
            [Out] byte[] serials,
            int maxCount);

        /* ===== lifecycle ===== */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int nexlink_open(
            string serial,
            out IntPtr handle);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void nexlink_close(
            IntPtr handle);

        /* ===== command ===== */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int nexlink_cmd(
            IntPtr handle,
            ushort cmd,
            byte[] payload,
            ushort length,
            out NexLinkPacket resp,
            int timeoutMs);

        /* ===== async command ===== */
        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern int nexlink_send_async(
            IntPtr handle,
            byte cmd,
            byte[] payload,
            ushort length);

        /* ===== event ===== */
        public delegate void EventCallback(
            IntPtr user,
            ref NexLinkPacket pkt);

        [DllImport(DLL, CallingConvention = CallingConvention.Cdecl)]
        public static extern void nexlink_register_event(
            IntPtr handle,
            EventCallback cb,
            IntPtr user);
    }
}
