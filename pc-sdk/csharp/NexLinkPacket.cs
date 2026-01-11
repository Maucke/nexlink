using System.Runtime.InteropServices;

namespace NexLink
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct NexLinkPacket
    {
        public byte magic;
        public byte type;
        public byte cmd;
        public ushort seq;
        public ushort length;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)]
        public byte[] payload;
    }

    public enum PacketType : byte
    {
        Cmd   = 0x01,
        Resp  = 0x02,
        Event = 0x03
    }
}
