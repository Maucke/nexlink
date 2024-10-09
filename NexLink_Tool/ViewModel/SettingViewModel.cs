using NexLink_Tool.Model;
using NexLinker;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NexLink_Tool.ViewModel
{
    internal class SettingViewModel : BindableBase
    {
        internal SettingViewModel()
        {
            Scan = new DelegateCommand<object>((o) =>
            {
                var deviceInfo = NexLink.ScanDevices();
                var DevicesCount = deviceInfo.Count;
                var tempDevicesItems = new ObservableCollection<NexDevice>();
                if (DevicesCount > 0)
                {
                    for (int i = 0; i < DevicesCount; i++)
                    {
                        tempDevicesItems.Add(new NexDevice()
                        {
                            Name = $"{deviceInfo[i].manufacturer}",
                            Index = i
                        });
                    }
                    Manager.ShowNoti($"当前设备数量：{DevicesCount}");
                }
                NexDevices = tempDevicesItems;
            });
            Control = new DelegateCommand<object>((o) =>
            {
                NexDevice device = o as NexDevice;
                if(device.IsConnect)
                {
                    foreach (var item in NexDevices)
                    {
                        if(item != device)
                            item.IsConnect = false;
                    }
                    var ret = Manager.nexLink.OpenDevice(device.Index);
                    if (ret)
                        device.IsConnect = true;
                    else
                    {
                        Manager.ShowNoti($"打开设备失败");
                        return;
                    }
                }
                else
                {
                    device.IsConnect = false;
                    Manager.nexLink.CloseDevice();
                }
            });
        }

        ObservableCollection<NexDevice> _NexDevices = new ObservableCollection<NexDevice>() { 
        };
        public ObservableCollection<NexDevice> NexDevices { get { return _NexDevices; } set { _NexDevices = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> Scan { get; set; }
        public DelegateCommand<object> Control { get; set; }
    }
}
