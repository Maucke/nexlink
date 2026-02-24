using ImageBppConverter;
using System;
using System.Collections.Generic;
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

        CmdHwReset = 0x0000,
        CmdPing = 0x0001,
        CmdGetVersion = 0x0002,
        CmdSyncTime = 0x0003,
        CmdLoopback = 0x0004,
        CmdKey = 0x0010,
        CmdGetDisplayInfo = 0x0011,

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
        CmdI2cConfig = 0x0311,
        CmdSpiTransfer = 0x0320,
        CmdSpiConfig = 0x0321,

        /* =========================
         * 0x0330 - 0x033F
         * UART
         * ========================= */

        CmdUartConfig = 0x0330,
        CmdUartWrite = 0x0331,

        EvtUartData = 0x0332,
        /* =========================
         * 0x0400 - 0x04FF
         * 高速数据 / Frame Buffer
         * ========================= */

        CmdFrameBegin = 0x0400,
        CmdFrameData = 0x0401,
        CmdFrameEnd = 0x0402,

        CmdFrameGet = 0x0403,
        EvtFrameUploadBegin = 0x0404,
        EvtFrameUploadData = 0x0405,
        EvtFrameUploadEnd = 0x0406,

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
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public class DisplayInfo
    {
        public ushort Width;
        public ushort Height;
        public TargetPixelFormat Bpp;
        public byte Refresh;

        public override string ToString()
        {
            return $"{Width}x{Height}, {Bpp}bpp, {Refresh}Hz";
        }
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public class DisplayInfoResult
    {
        public byte DisplayCount;
        public List<DisplayInfo> Displays = new List<DisplayInfo>();
        public override string ToString()
        {
            var sb = new System.Text.StringBuilder();

            sb.AppendLine($"DisplayCount: {DisplayCount}");

            if (Displays == null || Displays.Count == 0)
            {
                sb.AppendLine("Displays: <empty>");
                return sb.ToString();
            }

            sb.Append("Displays:");

            for (int i = 0; i < Displays.Count; i++)
            {
                sb.AppendLine();
                sb.Append($"  [{i}] {Displays[i]}");
            }

            return sb.ToString();
        }
    }

    public enum PacketType : byte
    {
        Cmd   = 0x01,
        Resp  = 0x02,
        Event = 0x03
    }
    public enum NexLinkError : byte
    {
        /* =========================
         * 0x00 - 0x0F 通用错误
         * ========================= */
        Ok = 0x00,
        Unsupported = 0x01,
        InvalidParam = 0x02,
        Busy = 0x03,
        NotReady = 0x04,
        NoMem = 0x05,
        Timeout = 0x06,
        Internal = 0x0F,

        /* =========================
         * 0x10 - 0x1F UART 错误
         * ========================= */
        UartOverrun = 0x10,
        UartFraming = 0x11,
        UartParity = 0x12,
        UartDma = 0x13,

        /* =========================
         * 0x20 - 0x2F I2C 错误
         * ========================= */
        I2cNack = 0x20,
        I2cBus = 0x21,
        I2cArbitration = 0x22,
        I2cOverrun = 0x23,
        I2cDma = 0x24,
        I2cTimeout = 0x25,

        /* =========================
         * 0x30 - 0x3F SPI 错误
         * ========================= */
        SpiOverrun = 0x30,
        SpiModeFault = 0x31,
    }

    public class UartDataEvent
    {
        public byte UartId;
        public byte[] Data;
    }
}
