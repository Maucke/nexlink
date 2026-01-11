using System;
using NexLink;

class Program
{
    static void Main()
    {
        var devices = NexLinkManager.Scan();

        if (devices.Count == 0)
        {
            Console.WriteLine("No NexLink device found");
            return;
        }

        var dev = NexLinkManager.Open(devices[0]);
        try
        {
            dev.OnEvent += pkt =>
            {
                Console.WriteLine(
                    $"EVENT cmd=0x{pkt.cmd:X2}, len={pkt.length}");
            };

            Console.WriteLine($"Connected: {dev.Serial}");

            long offset = dev.SyncTimeMs();
            Console.WriteLine($"Time offset(ms): {offset}");

            var resp = dev.SendCommand(0x01);
            Console.WriteLine($"RESP seq={resp.seq}");
        }
        finally
        {
            dev.Dispose();
        }

    }
}
