using NexLink;
using NexLink_Tool.Model;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using Wpf.Ui.Controls;

namespace NexLink_Tool.ViewModel
{
    internal class SettingViewModel : BindableBase
    {
        IReadOnlyList<NexLinkDeviceInfo> devices;
        internal SettingViewModel()
        {
            Scan = new DelegateCommand<object>((o) =>
            {
                CleanupDevice();
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
            Control = new DelegateCommand<object>((o) =>
            {
                NexDevice device = o as NexDevice;
                if (device == null) return;
                if(device.IsConnect)
                {
                    var Logs = Manager.homeViewModel.Logs;
                    Logs.Clear();
                    foreach (var nexdevice in NexDevices)
                    {
                        if (nexdevice != device)
                            nexdevice.IsConnect = false;
                    }
                    CleanupDevice();
                    try
                    {
                        Manager.dev = NexLinkManager.Open(device.Serial);
                        var version = Manager.dev.GetVersion();
                        device.Description = $"Version：{version}";
                        long offset = Manager.dev.SyncTimeMs();
                        Console.WriteLine($"Time offset(ms): {offset}");

                        Manager.dev.OnEvent += Dev_OnEvent;
                    }
                    catch (Exception e)
                    {
                        Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                        device.IsConnect = false;
                        CleanupDevice();
                    }
                }
                else
                {
                    device.Description = string.Empty;
                    CleanupDevice();
                }
            });
            Task.Run(async () =>
            {
                await Task.Delay(500);
                Scan.Execute(null);
                var defaultDevice = NexDevices.FirstOrDefault();
                if (defaultDevice == null) return;
                defaultDevice.IsConnect = true;
                Control.Execute(defaultDevice);
            });
        }
        private void CleanupDevice()
        {
            if (Manager.dev != null)
            {
                Manager.dev.OnEvent -= Dev_OnEvent;
                Manager.dev.Dispose();
                Manager.dev = null;
            }
        }


        private void Dev_OnEvent(NexLinkPacket pkt)
        {
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
                    break;

                case NexLinkCmd.EvtFrameBegin:
                    break;
            }
        }


        ObservableCollection<NexDevice> _NexDevices = new ObservableCollection<NexDevice>() { 
        };
        public ObservableCollection<NexDevice> NexDevices { get { return _NexDevices; } set { _NexDevices = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> Scan { get; set; }
        public DelegateCommand<object> Control { get; set; }
    }
}
