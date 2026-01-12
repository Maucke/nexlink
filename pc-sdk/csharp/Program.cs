using System;
using System.Threading;
using System.Threading.Tasks;
using NexLink;

class Program
{
    static void Main()
    {
        var devices = NexLinkManager.Scan();

        if (devices.Count == 0)
        {
            Console.WriteLine("No NexLink device found");
            Console.ReadKey();
            return;
        }

        foreach (var device in devices)
        {
            Console.WriteLine($"Finded: {device}");
        }

        var dev = NexLinkManager.Open(devices[0]);
        try
        {
            dev.OnEvent += pkt =>
            {
                switch (pkt.cmd)
                {
                    case NexLinkCmd.EvtLog:
                        Console.WriteLine(
                            $"LOG: {System.Text.Encoding.UTF8.GetString(pkt.payload, 0, pkt.length)}");
                        break;

                    case NexLinkCmd.EvtHeartbeat:
                        Console.WriteLine("Heartbeat received");
                        break;

                    case NexLinkCmd.EvtFrameBegin:
                        Console.WriteLine("Frame begin");
                        break;
                }
            };

            Console.WriteLine($"Connected: {dev.Serial}");

            while (true)
            {
                Console.ReadKey();
                try
                {
                    var resp = dev.SendCommand(NexLinkCmd.CmdPing);
                    Console.WriteLine($"RESP seq={resp.seq}");

                    long offset = dev.SyncTimeMs();
                    Console.WriteLine($"Time offset(ms): {offset}");
                }
                catch (Exception e)
                {
                    Console.WriteLine($"{e.Message}");
                }
                Thread.Sleep(100);
            }

        }
        finally
        {
            dev.Dispose();
        }
    }
}
