using NexLink;
using System;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

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
            Console.WriteLine($"Finded: {device.Serial}, {device.Product}");
        }

        var dev = NexLinkManager.Open(devices[0].Serial);
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

            try
            {
                var resp = dev.SendCommand(NexLinkCmd.CmdGetVersion);

                var data = NexLinkManager.GetRespData(resp);

                var version = NexLinkManager.BytesToStruct<NexLinkVersion>(data.ToArray());

                Console.WriteLine($"NexLink version: {version}");

            }
            catch (Exception e)
            {
                Console.WriteLine($"{e.Message}");
            }

            while (true)
            {
                Console.ReadKey();
                try
                {
                    var data = Enumerable.Range(0, 1000).Select(i => (byte)i).ToArray();

                    var sw = Stopwatch.StartNew();
                    //var resp = dev.SendCommand(NexLinkCmd.CmdPing);
                    var echoed = dev.Loopback(data);

                    Console.WriteLine(
                        echoed.SequenceEqual(data)
                            ? "Loopback OK"
                            : "Data corrupted");
                    sw.Stop();

                    double us = sw.ElapsedTicks * 1_000_000.0 / Stopwatch.Frequency;

                    Console.WriteLine(
                        $"RESP time = {us:F1} us"
                    );

                    long offset = dev.SyncTimeMs();
                    Console.WriteLine($"Time offset(ms): {offset}");
                }
                catch (Exception e)
                {
                    Console.WriteLine($"{e.Message}");
                }
                Thread.Sleep(1);
            }

        }
        finally
        {
            dev.Dispose();
        }
    }
}
