using System;
using System.Threading;

namespace NexLink
{
    public class NexLinkDevice : IDisposable
    {
        private readonly IntPtr _handle;
        private readonly string _serial;

        private readonly NexLinkNative.EventCallback _eventCb;

        public string Serial => _serial;

        public event Action<NexLinkPacket> OnEvent;

        internal NexLinkDevice(IntPtr handle, string serial)
        {
            _handle = handle;
            _serial = serial;

            _eventCb = OnNativeEvent;
            NexLinkNative.nexlink_register_event(
                _handle, _eventCb, IntPtr.Zero);
        }

        private void OnNativeEvent(
            IntPtr user,
            ref NexLinkPacket pkt)
        {
            OnEvent?.Invoke(pkt);
        }

        /* ========= 通用 CMD ========= */

        public NexLinkPacket SendCommand(
            NexLinkCmd cmd,
            byte[] payload = null,
            int timeoutMs = 1000)
        {
            if (payload == null)
                payload = Array.Empty<byte>();

            int rc = NexLinkNative.nexlink_cmd(
                _handle,
                (ushort)cmd,
                payload,
                (ushort)payload.Length,
                out var resp,
                timeoutMs);

            if (rc != 0)
                throw new TimeoutException(
                    $"CMD {cmd} timeout");

            return resp;
        }

        /* ========= Time Sync（普通 CMD） ========= */

        public long SyncTimeMs()
        {
            long pcTime =
                DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();

            byte[] payload =
                BitConverter.GetBytes(pcTime);

            var resp = SendCommand(
                NexLinkCmd.CmdSyncTime, payload, 1000);

            long mcuTime =
                BitConverter.ToInt64(resp.payload, 0);

            return mcuTime - pcTime;
        }

        /* ========= 示例业务 ========= */

        public byte[] ReadPeripheral(byte addr)
        {
            var resp = SendCommand(NexLinkCmd.CmdI2cTransfer, new[] { addr });
            byte[] data = new byte[resp.length];
            Array.Copy(resp.payload, data, resp.length);
            return data;
        }

        public void Dispose()
        {
            NexLinkNative.nexlink_close(_handle);
        }
    }
}
