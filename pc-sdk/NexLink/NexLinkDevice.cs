using ImageBppConverter;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Threading;
using ImageConverter = ImageBppConverter.ImageConverter;

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

            if (rc < 0)
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

            if (rc < 0)
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
        public DisplayInfoResult GetDisplayInfo()
        {
            var resp = SendCommand(
                NexLinkCmd.CmdGetDisplayInfo,
                null,
                1000);

            if (resp.payload == null || resp.payload.Length <= 1)
                throw new Exception("Invalid response");

            if (resp.payload[0] != 0)
                throw new Exception("Device returned error");

            int offset = 1;

            var result = new DisplayInfoResult();

            result.DisplayCount = resp.payload[offset++];

            for (int i = 0; i < result.DisplayCount; i++)
            {
                var info = new DisplayInfo();

                info.Width = BitConverter.ToUInt16(resp.payload, offset);
                offset += 2;

                info.Height = BitConverter.ToUInt16(resp.payload, offset);
                offset += 2;

                info.Bpp = (TargetPixelFormat)resp.payload[offset++];
                info.Refresh = resp.payload[offset++];

                result.Displays.Add(info);
            }

            return result;
        }
        public NexLinkVersion GetVersion()
        {
            var resp = SendCommand(NexLinkCmd.CmdGetVersion, null, 1000);

            if (resp.payload == null || resp.payload.Length <= 1)
                throw new InvalidOperationException("Invalid version response");

            return NexLinkManager.BytesToStruct<NexLinkVersion>(
                resp.payload.AsSpan(1).ToArray());
        }
        public void SendFrame(ImageResult image, TargetPixelFormat bpp, int chunkSize = 1000)
        {
            // ---------- 1. FrameStart ----------
            var startPayload = new byte[5];

            BitConverter.GetBytes((ushort)image.Width)
                .CopyTo(startPayload, 0);

            BitConverter.GetBytes((ushort)image.Height)
                .CopyTo(startPayload, 2);

            startPayload[4] = (byte)bpp; 

            SendCommand(NexLinkCmd.CmdFrameBegin, startPayload, 1000);

            // ---------- 2. FrameData ----------
            int total = image.Data.Length;
            int offset = 0;

            while (offset < total)
            {
                int size = Math.Min(chunkSize, total - offset);

                byte[] payload = new byte[size];

                Array.Copy(image.Data, offset, payload, 0, size);

                SendAsync(NexLinkCmd.CmdFrameData, payload);

                offset += size;
            }

            // ---------- 3. FrameEnd ----------
            SendCommand(NexLinkCmd.CmdFrameEnd, null, 1000);
        }
        public void GetFrame()
        {
            SendCommand(NexLinkCmd.CmdFrameGet, null, 1000);
        }
        public void ConfigureI2c(
    byte busId,
    uint clockHz)
        {
            byte[] payload = new byte[5];

            payload[0] = busId;
            BitConverter.GetBytes(clockHz)
                .CopyTo(payload, 1);

            SendCommand(NexLinkCmd.CmdI2cConfig, payload, 1000);
        }
        public byte[] I2cTransfer(
    byte busId,
    byte slaveAddr,
    byte[] writeData,
    ushort readLength,
    bool sendStop = true,
    bool repeatedStart = false,
    int timeoutMs = 1000)
        {
            if (writeData == null)
                writeData = Array.Empty<byte>();

            byte flags = 0;
            if (sendStop) flags |= 0x01;
            if (repeatedStart) flags |= 0x02;

            int payloadLen =
                1 +  // busId
                1 +  // slave
                1 +  // flags
                2 +  // write len
                2 +  // read len
                writeData.Length;

            byte[] payload = new byte[payloadLen];

            int offset = 0;

            payload[offset++] = busId;
            payload[offset++] = slaveAddr;
            payload[offset++] = flags;

            BitConverter.GetBytes((ushort)writeData.Length)
                .CopyTo(payload, offset);
            offset += 2;

            BitConverter.GetBytes(readLength)
                .CopyTo(payload, offset);
            offset += 2;

            if (writeData.Length > 0)
            {
                Array.Copy(writeData, 0, payload, offset, writeData.Length);
            }

            var resp = SendCommand(
                NexLinkCmd.CmdI2cTransfer,
                payload,
                timeoutMs);

            // payload[0] 已经被 CheckRespError 校验
            byte[] result = new byte[resp.length - 1];
            Array.Copy(resp.payload, 1, result, 0, result.Length);

            return result;
        }
        public void ConfigureSpi(
    byte busId,
    uint clockHz,
    byte mode,      // 0-3
    byte bitOrder)  // 0=MSB 1=LSB
        {
            byte[] payload = new byte[7];

            int offset = 0;

            payload[offset++] = busId;

            BitConverter.GetBytes(clockHz)
                .CopyTo(payload, offset);
            offset += 4;

            payload[offset++] = mode;
            payload[offset++] = bitOrder;

            SendCommand(NexLinkCmd.CmdSpiConfig, payload, 1000);
        }
        public byte[] SpiTransfer(
    byte busId,
    byte csId,
    byte[] txData,
    bool autoCs = true,
    int timeoutMs = 1000)
        {
            if (txData == null)
                txData = Array.Empty<byte>();

            byte flags = 0;
            if (autoCs) flags |= 0x01;

            int payloadLen =
                1 +  // busId
                1 +  // csId
                1 +  // flags
                2 +  // length
                txData.Length;

            byte[] payload = new byte[payloadLen];

            int offset = 0;

            payload[offset++] = busId;
            payload[offset++] = csId;
            payload[offset++] = flags;

            BitConverter.GetBytes((ushort)txData.Length)
                .CopyTo(payload, offset);
            offset += 2;

            if (txData.Length > 0)
                Array.Copy(txData, 0, payload, offset, txData.Length);

            var resp = SendCommand(
                NexLinkCmd.CmdSpiTransfer,
                payload,
                timeoutMs);

            byte[] rx = new byte[resp.length - 1];
            Array.Copy(resp.payload, 1, rx, 0, rx.Length);

            return rx;
        }
        public void ConfigureUart(
    byte uartId,
    uint baudrate,
    byte dataBits = 8,
    byte stopBits = 1,
    byte parity = 0,
    byte flowCtrl = 0,
    int timeoutMs = 1000)
        {
            byte[] payload = new byte[9];

            int offset = 0;

            payload[offset++] = uartId;

            BitConverter.GetBytes(baudrate)
                .CopyTo(payload, offset);
            offset += 4;

            payload[offset++] = dataBits;
            payload[offset++] = stopBits;
            payload[offset++] = parity;
            payload[offset++] = flowCtrl;

            SendCommand(
                NexLinkCmd.CmdUartConfig,
                payload,
                timeoutMs);
        }
        public void UartWrite(
    byte uartId,
    byte[] data,
    int timeoutMs = 1000)
        {
            if (data == null)
                data = Array.Empty<byte>();

            int payloadLen =
                1 +   // uartId
                2 +   // length
                data.Length;

            byte[] payload = new byte[payloadLen];

            int offset = 0;

            payload[offset++] = uartId;

            BitConverter.GetBytes((ushort)data.Length)
                .CopyTo(payload, offset);
            offset += 2;

            if (data.Length > 0)
                Array.Copy(data, 0, payload, offset, data.Length);

            SendCommand(
                NexLinkCmd.CmdUartWrite,
                payload,
                timeoutMs);
        }

        public UartDataEvent ParseUartEvent(NexLinkPacket pkt)
        {
            if ((NexLinkCmd)pkt.cmd != NexLinkCmd.EvtUartData)
                return null;

            if (pkt.payload == null || pkt.payload.Length < 3)
                return null;

            int offset = 0;

            byte uartId = pkt.payload[offset++];

            ushort len = BitConverter.ToUInt16(pkt.payload, offset);
            offset += 2;

            if (pkt.payload.Length < offset + len)
                return null;

            byte[] data = new byte[len];
            Array.Copy(pkt.payload, offset, data, 0, len);

            return new UartDataEvent
            {
                UartId = uartId,
                Data = data
            };
        }

        ImageResult imageResult = new ImageResult(0, 0, null);
        List<byte> pictureBuff = new List<byte>();
        public Bitmap ParseFrameUploadEvent(NexLinkPacket pkt)
        {
            switch (pkt.cmd)
            {
                case NexLinkCmd.EvtFrameUploadBegin:
                    {
                        if (pkt.length != 5)
                            throw new Exception($"EvtFrameUploadBegin incorrect length: {pkt.length}");

                        ushort width = BitConverter.ToUInt16(pkt.payload, 0);
                        ushort height = BitConverter.ToUInt16(pkt.payload, 2);
                        TargetPixelFormat bpp =
                            (TargetPixelFormat)pkt.payload[4];

                        pictureBuff.Clear();

                        imageResult.Width = width;
                        imageResult.Height = height;

                        return null;
                    }

                case NexLinkCmd.EvtFrameUploadData:
                    {
                        if (pkt.payload != null && pkt.length > 0)
                            pictureBuff.AddRange(pkt.payload);

                        return null;
                    }

                case NexLinkCmd.EvtFrameUploadEnd:
                    {
                        imageResult.Data = pictureBuff.ToArray();

                        var bmp =
                            ImageConverter.Convert2bppToBitmap(imageResult);

                        return bmp;
                    }
            }

            return null;
        }

        public void ConfigureGpio(
            GpioPort port,
            GpioPin pins,
            GpioMode mode)
        {
            byte[] payload = new byte[4];

            int offset = 0;

            payload[offset++] = (byte)port;

            BitConverter.GetBytes((ushort)pins)
                .CopyTo(payload, offset);
            offset += 2;

            payload[offset++] = (byte)mode;

            SendCommand(
                NexLinkCmd.CmdGpioConfig,
                payload,
                1000);
        }

        public void WriteGpio(
            GpioPort port,
            GpioPin pins,
            bool value)
        {
            byte[] payload = new byte[4];

            int offset = 0;

            payload[offset++] = (byte)port;

            BitConverter.GetBytes((ushort)pins)
                .CopyTo(payload, offset);
            offset += 2;

            payload[offset++] = value ? (byte)1 : (byte)0;

            SendCommand(
                NexLinkCmd.CmdGpioWrite,
                payload,
                1000);
        }
        public bool ReadGpio(
            GpioPort port,
            GpioPin pins)
        {
            byte[] payload = new byte[3];

            int offset = 0;

            payload[offset++] = (byte)port;

            BitConverter.GetBytes((ushort)pins)
                .CopyTo(payload, offset);

            var resp = SendCommand(
                NexLinkCmd.CmdGpioRead,
                payload,
                1000);

            if (resp.payload == null || resp.payload.Length < 2)
                throw new Exception("Invalid GPIO read response");

            return resp.payload[1] != 0;
        }
        public void Dispose()
        {
            NexLinkNative.nexlink_close(_handle);
        }
    }
}
