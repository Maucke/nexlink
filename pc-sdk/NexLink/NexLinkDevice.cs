using System;
using System.Threading;

namespace NexLink
{
    public class NexLinkDevice : IDisposable
    {
        public const int MaxPayloadSize = 1000;

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
        private static void CheckRespError(NexLinkPacket resp)
        {
            if (resp.payload == null || resp.payload.Length < 1)
                throw new InvalidOperationException("Invalid response payload");

            var err = (NexLinkError)resp.payload[0];

            if (err != NexLinkError.Ok)
                throw new NexLinkException(
                    err,
                    $"CMD {resp.cmd} failed: {err}");
        }
        /* ========= 通用 CMD ========= */

        public NexLinkPacket SendCommand(
            NexLinkCmd cmd,
            byte[] payload = null,
            int timeoutMs = 1000)
        {
            if (payload == null)
                payload = Array.Empty<byte>();

            if (payload.Length > MaxPayloadSize)
                throw new ArgumentOutOfRangeException(
                    $"Payload too large: {payload.Length}, max = {MaxPayloadSize}");

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

            CheckRespError(resp);

            return resp;
        }
        public void SendAsync(
            NexLinkCmd cmd,
            byte[] payload = null)
        {
            if (payload == null)
                payload = Array.Empty<byte>();

            if (payload.Length > MaxPayloadSize)
                throw new ArgumentOutOfRangeException(
                    $"Payload too large: {payload.Length}, max = {MaxPayloadSize}");

            int rc = NexLinkNative.nexlink_send_async(
                _handle,
                (ushort)cmd,
                payload,
                (ushort)payload.Length);

            if (rc != 0)
                throw new TimeoutException(
                    $"CMD {cmd} timeout");
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
                BitConverter.ToInt64(resp.payload, 1);

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
        public byte[] Loopback(byte[] data)
        {
            var resp = SendCommand(
                NexLinkCmd.CmdLoopback,
                data,
                timeoutMs: 1000
            );

            ushort err = BitConverter.ToUInt16(resp.payload, 0);
            if (err != 0)
                throw new Exception($"Loopback failed: {err}");

            byte[] echoed = new byte[resp.length - 1];
            Array.Copy(resp.payload, 1, echoed, 0, echoed.Length);
            return echoed;
        }

        public void Dispose()
        {
            NexLinkNative.nexlink_close(_handle);
        }
    }
}
