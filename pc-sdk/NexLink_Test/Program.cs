using NexLink;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;

class NexLink_Test
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
            dev.Dispose();
            dev = NexLinkManager.Open(devices[0].Serial);
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
                //Console.ReadKey();
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
                //Thread.Sleep(1);
            }

            // 测试配置
            int testDurationMs = 1000; // 测试时长：1秒
            byte[] testData = Enumerable.Range(0, 1000).Select(i => (byte)i).ToArray(); // 单次回环数据：1000字节
            int singleDataLen = testData.Length; // 单次回环字节数

            Console.WriteLine($"开始1秒回环测试，单次回环数据量：{singleDataLen} 字节");
            Console.WriteLine("----------------------------------------");

            // 初始化统计变量
            int loopCount = 0; // 成功回环次数
            long totalBytes = 0; // 总回环字节数
            List<double> singleLoopTimesUs = new List<double>(); // 单次耗时（微秒）
            bool testRunning = true;

            // 启动1秒计时
            Stopwatch testStopwatch = Stopwatch.StartNew();

            try
            {
                while (testRunning)
                {
                    // 检查是否超出1秒测试窗口
                    if (testStopwatch.ElapsedMilliseconds >= testDurationMs)
                    {
                        testRunning = false;
                        break;
                    }

                    try
                    {
                        var sw = Stopwatch.StartNew();
                        // 执行回环测试
                        var echoed = dev.Loopback(testData);
                        sw.Stop();

                        // 校验数据完整性
                        bool isDataOk = echoed.SequenceEqual(testData);
                        if (!isDataOk)
                        {
                            Console.WriteLine($"第 {loopCount + 1} 次回环：数据损坏！");
                            continue; // 数据损坏则不计入统计
                        }

                        // 统计有效回环
                        loopCount++;
                        totalBytes += singleDataLen;

                        // 计算单次耗时（微秒）
                        double us = sw.ElapsedTicks * 1_000_000.0 / Stopwatch.Frequency;
                        singleLoopTimesUs.Add(us);

                        //// 同步时间（保留原有逻辑，可根据需要注释）
                        //long offset = dev.SyncTimeMs();
                        //if (loopCount % 100 == 0) // 每100次打印一次偏移，避免刷屏
                        //{
                        //    Console.WriteLine($"第 {loopCount} 次回环 - 时间偏移(ms)：{offset}");
                        //}
                    }
                    catch (Exception innerEx)
                    {
                        Console.WriteLine($"单次回环异常：{innerEx.Message}");
                        // 异常时继续测试，不终止整体流程
                        continue;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"测试主流程异常：{ex.Message}");
            }
            finally
            {
                testStopwatch.Stop();
            }

            // 输出1秒测试汇总结果
            Console.WriteLine("----------------------------------------");
            Console.WriteLine($"1秒回环测试结果（实际测试时长：{testStopwatch.ElapsedMilliseconds:F1} ms）");
            Console.WriteLine($"✅ 成功回环次数：{loopCount} 次");
            Console.WriteLine($"📊 总回环数据量：{totalBytes} 字节（{totalBytes / 1024:F2} KB）");
            if (loopCount > 0)
            {
                Console.WriteLine($"⏱️  平均单次耗时：{singleLoopTimesUs.Average():F1} 微秒");
                Console.WriteLine($"⚡ 最大单次耗时：{singleLoopTimesUs.Max():F1} 微秒");
                Console.WriteLine($"⚡ 最小单次耗时：{singleLoopTimesUs.Min():F1} 微秒");
                Console.WriteLine($"📈 每秒数据吞吐量：{totalBytes / (testStopwatch.ElapsedMilliseconds / 1000.0):F0} 字节/秒");
            }
            else
            {
                Console.WriteLine("❌ 无有效回环数据");
            }

            Console.ReadKey();
        }
        finally
        {
            dev.Dispose();
        }
    }
}

