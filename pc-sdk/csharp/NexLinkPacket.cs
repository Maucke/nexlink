using System;
using System.Runtime.InteropServices;

namespace NexLink
{
    public sealed class NexLinkDeviceInfo
    {
        public string Serial { get; set; }
        public string Product { get; set; }
    }

    public enum NexLinkCmd : ushort
    {
        /* =========================
         * 0x0000 - 0x00FF
         * 核心 / 系统
         * ========================= */

        CmdPing = 0x0001,
        CmdGetVersion = 0x0002,
        CmdSyncTime = 0x0003,
        CmdLoopback = 0x0004,

        /* =========================
         * 0x0100 - 0x01FF
         * 日志 / 调试（EVENT 为主）
         * ========================= */

        EvtLog = 0x0100,
        EvtWarn = 0x0101,
        EvtError = 0x0102,

        /* =========================
         * 0x0200 - 0x02FF
         * 状态 / 监控
         * ========================= */

        EvtStatus = 0x0200,
        EvtHeartbeat = 0x0201,

        /* =========================
         * 0x0300 - 0x03FF
         * 外设 / 控制
         * ========================= */

        CmdGpioWrite = 0x0300,
        CmdGpioRead = 0x0301,

        CmdI2cTransfer = 0x0310,
        CmdSpiTransfer = 0x0320,

        /* =========================
         * 0x0400 - 0x04FF
         * 高速数据 / Frame Buffer
         * ========================= */

        CmdFrameStart = 0x0400,
        CmdFrameData = 0x0401,
        CmdFrameEnd = 0x0402,

        EvtFrameBegin = 0x0480,
        EvtFrameData = 0x0481,
        EvtFrameEnd = 0x0482,

        /* =========================
         * 0x7F00 - 0x7FFF
         * 保留给用户自定义
         * ========================= */

        UserBase = 0x7F00
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct NexLinkPacket
    {
        public byte magic;
        public byte type;
        public NexLinkCmd cmd;
        public ushort seq;
        public ushort length;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 1024)]
        public byte[] payload;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct NexLinkVersion
    {
        public byte Major;
        public byte Minor;
        public ushort Patch;
        public uint Build;
        public override string ToString()
            => $"{Major}.{Minor}.{Patch} (build {Build})";
    }

    public enum PacketType : byte
    {
        Cmd   = 0x01,
        Resp  = 0x02,
        Event = 0x03
    }
    public enum NexLinkError : byte
    {
        Ok = 0x00,
        Unsupported = 0x01,
        InvalidParam = 0x02,
        Busy = 0x03,
        NotReady = 0x04,
        Internal = 0x7F,
    }
}
