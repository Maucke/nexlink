using Hexconverters;
using ImageBppConverter;
using NexLink;
using NexLink_Tool.Model;
using NexLink_Tool.Page;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Markup;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using Wpf.Ui.Controls;
using static System.Net.Mime.MediaTypeNames;
using ImageConverter = ImageBppConverter.ImageConverter;

namespace NexLink_Tool.ViewModel
{
    internal class SettingViewModel : BindableBase
    {
        IReadOnlyList<NexLinkDeviceInfo> devices;
        internal SettingViewModel()
        {
            Scan = new DelegateCommand<object>(async (o) =>
            {

                await CleanupDevice();
                devices = NexLinkManager.Scan();
                var DevicesCount = devices.Count;
                var tempDevicesItems = new ObservableCollection<NexDevice>();
                if (DevicesCount > 0)
                {
                    for (int i = 0; i < DevicesCount; i++)
                    {
                        tempDevicesItems.Add(new NexDevice()
                        {
                            Product = devices[i].Product,
                            Serial = devices[i].Serial,
                        });
                    }
                    Manager.ShowNoti($"当前设备数量：{DevicesCount}");
                }
                NexDevices = tempDevicesItems;
            });
            Control = new DelegateCommand<object>(async (o) =>
            {
                NexDevice device = o as NexDevice;
                if (device == null) return;
                if(device.IsConnect)
                {
                    foreach (var nexdevice in NexDevices)
                    {
                        if (nexdevice != device)
                        {
                            nexdevice.IsConnect = false;
                            nexdevice.Description = string.Empty;
                        }
                    }
                    await CleanupDevice();
                    try
                    {
                        Manager.dev = NexLinkManager.Open(device.Serial);
                        await Task.Delay(100);
                        var version = Manager.dev.GetVersion();
                        device.Description = $"Version：{version}";
                        long offset = Manager.dev.SyncTimeMs();
                        Console.WriteLine($"Time offset(ms): {offset}");

                        var Logs = Manager.homeViewModel.Logs;
                        Logs.Clear();
                        Manager.ShowNoti($"{device.Product} has been connected!");
                        Manager.dev.OnEvent += Dev_OnEvent;

                    }
                    catch (Exception e)
                    {
                        Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                        device.IsConnect = false;
                        // CleanupDevice();
                    }
                    if (Manager.dev != null && Manager.homeViewModel.IsSyncing)
                    {
                        try
                        {
                            var info = Manager.dev?.GetDisplayInfo();
                            if (info.DisplayCount > 0)
                                Manager.dev?.SendCommand(NexLinkCmd.CmdFrameGet, [0xFF]);
                        }
                        catch (Exception e)
                        {
                            Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                        }
                    }
                }
                else
                {
                    device.Description = string.Empty;

                    await CleanupDevice();
                }
            });
            Task.Run(async () =>
            {
                await Task.Delay(500);
                Scan.Execute(null);
                var defaultDevice = NexDevices.FirstOrDefault();
                if (defaultDevice == null) return;
                defaultDevice.IsConnect = true;
                Manager.BeginInvokeAction(() => Control.Execute(defaultDevice));
            });
        }
        private async Task CleanupDevice()
        {
            if (Manager.dev != null)
            {
                Manager.dev.OnEvent -= Dev_OnEvent;
                Manager.dev.Dispose();
                Manager.dev = null;
                Manager.homeViewModel.DisplayImage = null;
                _heartbeatSupported = false;
                _heartbeatLost = false;
                await Task.Delay(200);
            }
        }
        public static BitmapImage ConvertToImageSource(Bitmap bitmap)
        {
            using (var ms = new MemoryStream())
            {
                bitmap.Save(ms, System.Drawing.Imaging.ImageFormat.Png);
                ms.Position = 0;

                var image = new BitmapImage();
                image.BeginInit();
                image.CacheOption = BitmapCacheOption.OnLoad;
                image.StreamSource = ms;
                image.EndInit();
                image.Freeze(); // 重要：跨线程安全

                return image;
            }
        }
        private bool _heartbeatSupported = false;   // 是否检测到心跳
        private bool _heartbeatLost = false;
        private DateTime _lastHeartbeatTime;        // 最近一次心跳
        private Timer _heartbeatTimer;              // 超时检测定时器

        private void StartHeartbeatMonitor()
        {
            _lastHeartbeatTime = DateTime.Now;

            _heartbeatTimer?.Dispose();

            _heartbeatTimer = new Timer(async _ =>
            {
                if (!_heartbeatSupported)
                    return;

                var diff = DateTime.Now - _lastHeartbeatTime;

                if (diff.TotalSeconds > 10)
                {
                    if (!_heartbeatLost)
                    {
                        _heartbeatLost = true;
                        var nexdev = NexDevices.FirstOrDefault(x => x.Serial == Manager.dev?.Serial);
                        nexdev?.IsConnect = false;
                        nexdev?.Description = "";
                        await CleanupDevice();
                        Manager.ShowNoti(
                            "Heartbeat timeout!",
                            ControlAppearance.Caution);
                    }

                }
                else
                {
                    _heartbeatLost = false;
                }

            }, null, 1000, 1000);
        }

        private void Dev_OnEvent(NexLinkPacket pkt)
        {
            try
            {
                var dev = Manager.dev;
                if (dev == null)
                {
                    throw new Exception("Device not connected");
                }

                switch (pkt.cmd)
                {
                    case NexLinkCmd.EvtLog:
                        {
                            var text = Encoding.UTF8
                                .GetString(pkt.payload, 0, pkt.length)
                                .TrimEnd('\r', '\n');
                            Manager.AppendLog("[EVENT] " + text, LogLevel.Info);
                        }
                        break;
                    case NexLinkCmd.EvtWarn:
                        {
                            var text = Encoding.UTF8
                                .GetString(pkt.payload, 0, pkt.length)
                                .TrimEnd('\r', '\n');
                            Manager.AppendLog("[EVENT] " + text, LogLevel.Warn);
                        }
                        break;
                    case NexLinkCmd.EvtError:
                        {
                            var text = Encoding.UTF8
                                .GetString(pkt.payload, 0, pkt.length)
                                .TrimEnd('\r', '\n');
                            Manager.AppendLog("[EVENT] " + text, LogLevel.Error);
                        }
                        break;
                    case NexLinkCmd.EvtHeartbeat:
                        if (!_heartbeatSupported)
                        {
                            _heartbeatSupported = true;
                            StartHeartbeatMonitor();
                            Console.WriteLine("Heartbeat supported.");
                        }

                        _lastHeartbeatTime = DateTime.Now;

                        break;

                    default:
                        var bmp = dev.ParseFrameUploadEvent(pkt);
                        if (bmp != null)
                        {
#if false
                            string exePath = AppDomain.CurrentDomain.BaseDirectory;
                            // 生成文件名
                            string filePath = Path.Combine(exePath,
                                $"Screenshoot_{DateTime.Now:yyyyMMdd_HHmmss}.bmp");

                            // 保存
                            bmp.Save(filePath, System.Drawing.Imaging.ImageFormat.Bmp); 
                            bmp.Dispose();
#else
                            Manager.BeginInvokeAction(new Action(() =>
                            Manager.homeViewModel.DisplayImage = ConvertToImageSource(bmp)));
#endif
                        }
                        var uartdata = dev.ParseUartEvent(pkt);
                        if (uartdata != null)
                        {
                            Manager.peripheralViewModel.AppendUartLog($"[{DateTime.Now:HH:mm:ss.fff}-{uartdata.UartId}] RX: {Hexstring.ToString([.. uartdata.Data])}\r\n");
                            //Manager.AppendLog("[EVENT] " + $"Uart{uartdata.UartId}: {Hexstring.ToString(uartdata.Data)}", LogLevel.Info);
                        }
                        break;

                }
            }
            catch (Exception e)
            {
                Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
            }
        }


        ObservableCollection<NexDevice> _NexDevices = new ObservableCollection<NexDevice>() { 
        };
        public ObservableCollection<NexDevice> NexDevices { get { return _NexDevices; } set { _NexDevices = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> Scan { get; set; }
        public DelegateCommand<object> Control { get; set; }
    }
}
