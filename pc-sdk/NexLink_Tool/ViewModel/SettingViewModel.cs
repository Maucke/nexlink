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
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Media.Imaging;
using Wpf.Ui.Appearance;
using Wpf.Ui.Controls;
using ImageConverter = ImageBppConverter.ImageConverter;

namespace NexLink_Tool.ViewModel
{
    internal class SettingViewModel : BindableBase
    {
        IReadOnlyList<NexLinkDeviceInfo> devices;

        private readonly object _connectLock = new object();
        private bool _connecting;
        private bool _cleaningUp;
        private NexLinkDevice _currentDevice;
        private Timer _heartbeatTimer;
        private DateTime _lastHeartbeatTime;
        private bool _heartbeatSupported;
        private bool _heartbeatLost;
        private static readonly object _eventLock = new object();

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

                lock (_connectLock)
                {
                    if (_connecting) return;
                    _connecting = true;
                }

                await Task.Run(async () =>
                {
                    try
                    {
                        if (device.IsConnect)
                            await ConnectDevice(device);
                        else
                            await DisconnectDevice(device);
                    }
                    finally
                    {
                        lock (_connectLock) { _connecting = false; }
                    }
                });
            });

            ThemeSwitch = new DelegateCommand<object>((o) =>
            {
                if ((bool)o)
                    ApplicationThemeManager.Apply(ApplicationTheme.Dark);
                else
                    ApplicationThemeManager.Apply(ApplicationTheme.Light);
            });
        }

        private async Task ConnectDevice(NexDevice device)
        {
            // Clear previous connection on UI thread
            await Manager.BeginInvokeActionAsync(() =>
            {
                foreach (var nd in NexDevices)
                {
                    if (nd != device)
                    {
                        nd.IsConnect = false;
                        nd.Description = string.Empty;
                    }
                }
            });

            await CleanupDevice();

            NexLinkDevice newDev = null;
            try
            {
                newDev = NexLinkManager.Open(device.Serial);
                string ver = $"Version：{newDev.GetVersion()}";
                await Manager.BeginInvokeActionAsync(() => device.Description = ver);

                lock (_eventLock)
                {
                    _currentDevice = newDev;
                    Manager.dev = newDev;
                    newDev.OnEvent += Dev_OnEvent;
                }

                // If Sync Screen is enabled, trigger it now
                if (Manager.homeViewModel.IsSyncing)
                {
                    try
                    {
                        var info = newDev.GetDisplayInfo();
                        if (info.DisplayCount > 0)
                            newDev.SendCommand(NexLinkCmd.CmdFrameGet, [0xFF]);
                    }
                    catch { }
                }

                Manager.BeginInvokeAction(() =>
                {
                    Manager.homeViewModel.Logs.Clear();
                });

                _heartbeatSupported = false;
                _heartbeatLost = false;
                StartHeartbeatMonitor();

                Manager.ShowNoti($"{device.Product} has been connected!");
            }
            catch (Exception e)
            {
                newDev?.Dispose();
                Manager.dev = null;
                _currentDevice = null;
                await Manager.BeginInvokeActionAsync(() => device.IsConnect = false);
                Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
            }
        }

        private async Task DisconnectDevice(NexDevice device)
        {
            await CleanupDevice();
            await Manager.BeginInvokeActionAsync(() => device.Description = string.Empty);
        }

        private async Task CleanupDevice()
        {
            lock (_connectLock)
            {
                if (_cleaningUp) return;
                _cleaningUp = true;
            }

            try
            {
                // Stop heartbeat
                _heartbeatSupported = false;
                _heartbeatLost = false;
                _heartbeatTimer?.Dispose();
                _heartbeatTimer = null;

                // Close StreamOverlay
                await Manager.BeginInvokeActionAsync(() =>
                {
                    foreach (Window w in Application.Current.Windows)
                    {
                        if (w is StreamOverlay ov)
                            ov.Stop();
                    }
                });

                await Task.Delay(50);

                // Clean up device
                NexLinkDevice oldDev;
                lock (_eventLock)
                {
                    oldDev = _currentDevice;
                    _currentDevice = null;
                    Manager.dev = null;
                }

                if (oldDev != null)
                {
                    lock (_eventLock)
                    {
                        oldDev.OnEvent -= Dev_OnEvent;
                    }
                    oldDev.Dispose();
                }

                Manager.BeginInvokeAction(() =>
                {
                    Manager.homeViewModel.DisplayImage = null;
                });
            }
            finally
            {
                lock (_connectLock) { _cleaningUp = false; }
            }
        }

        private void StartHeartbeatMonitor()
        {
            _lastHeartbeatTime = DateTime.Now;
            _heartbeatTimer?.Dispose();

            _heartbeatTimer = new Timer(_ =>
            {
                if (!_heartbeatSupported)
                    return;

                var diff = DateTime.Now - _lastHeartbeatTime;

                if (diff.TotalSeconds > 10)
                {
                    if (!_heartbeatLost)
                    {
                        _heartbeatLost = true;

                        NexLinkDevice snapshot;
                        lock (_eventLock) { snapshot = _currentDevice; }

                        var nexdev = NexDevices.FirstOrDefault(x => x.Serial == snapshot?.Serial);
                        if (nexdev != null)
                        {
                            Manager.BeginInvokeAction(() =>
                            {
                                nexdev.IsConnect = false;
                                nexdev.Description = "";
                            });
                        }

                        _ = CleanupDevice();
                        Manager.ShowNoti("Heartbeat timeout!", ControlAppearance.Caution);
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
            NexLinkDevice dev;
            lock (_eventLock) { dev = _currentDevice; }
            if (dev == null) return;

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
                        Manager.BeginInvokeAction(() =>
                            Manager.homeViewModel.DisplayImage = ConvertToImageSource(bmp));
                    }
                    var uartdata = dev.ParseUartEvent(pkt);
                    if (uartdata != null)
                    {
                        Manager.peripheralViewModel.AppendUartLog(
                            $"[{DateTime.Now:HH:mm:ss.fff}-{uartdata.UartId}] RX: {Hexstring.ToString([.. uartdata.Data])}\r\n");
                    }
                    break;
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
                image.Freeze();
                return image;
            }
        }

        ObservableCollection<NexDevice> _NexDevices = new ObservableCollection<NexDevice>();
        public ObservableCollection<NexDevice> NexDevices { get { return _NexDevices; } set { _NexDevices = value; RaisePropertyChanged(); } }

        bool _ThemeDark;
        public bool ThemeDark
        { get { return _ThemeDark; } set { _ThemeDark = value; RaisePropertyChanged(); } }
        public DelegateCommand<object> Scan { get; set; }
        public DelegateCommand<object> Control { get; set; }
        public DelegateCommand<object> ThemeSwitch { get; set; }
    }
}
