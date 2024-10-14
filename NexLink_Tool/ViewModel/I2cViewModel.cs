using Hexconverters;
using NexLink_Tool.Model;
using NexLinker;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NexLink_Tool.ViewModel
{
    internal class I2cViewModel : BindableBase
    {
        internal I2cViewModel()
        {
            WriteRead = new DelegateCommand<object>((o) => {
                var cmd = o as NexI2cOperator;
                if (cmd == null) return;
                nex_i2c_request i2cRequest = new nex_i2c_request();
                try
                {
                    i2cRequest.deviceAddress = cmd.SlaveAddr;
                    i2cRequest.timeout = 500;
                    i2cRequest.dataWriteBuffer = new byte[64 - 8];
                    i2cRequest.dataWriteBuffer[0] = cmd.RegAddr;
                    i2cRequest.dataWriteLength = 1;
                    i2cRequest.dataReadLength = cmd.Size;
                    var readBytes = new byte[64];
                    var errLog = string.Empty;
                    var ret = Manager.nexLink.I2cWriteRead(i2cRequest, ref readBytes, ref errLog);
                    if (ret != LibUsbError.SUCCESS)
                    {
                        Manager.ShowNoti($"Ret:{ret}, Log:{errLog}", Wpf.Ui.Controls.ControlAppearance.Caution, 5);
                    }
                    else 
                        cmd.Data = ($"{Hexstring.ToString(readBytes)}");

                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e}");
                }
            });

            Write = new DelegateCommand<object>((o) => {
                var cmd = o as NexI2cOperator;
                if (cmd == null) return;
                nex_i2c_request i2cRequest = new nex_i2c_request();

                try
                {
                    var rawBytes = Hexstring.GetBytes(cmd.Data);

                    i2cRequest.deviceAddress = cmd.SlaveAddr;
                    i2cRequest.timeout = 50;
                    i2cRequest.dataWriteBuffer = new byte[64 - 8];
                    i2cRequest.dataWriteBuffer[0] = cmd.RegAddr;
                    Array.Copy(rawBytes, 0, i2cRequest.dataWriteBuffer, 1, rawBytes.Length);
                    i2cRequest.dataWriteLength = (ushort)(1 + rawBytes.Length);
                    i2cRequest.dataReadLength = 0;
                    var readBytes = new byte[64];
                    var errLog = string.Empty;
                    var ret = Manager.nexLink.I2cWriteRead(i2cRequest, ref readBytes, ref errLog);
                    if (ret != LibUsbError.SUCCESS)
                    {
                        Manager.ShowNoti($"Ret:{ret}, Log:{errLog}", Wpf.Ui.Controls.ControlAppearance.Caution, 5);
                    }

                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e}");
                }
            });

            Add = new DelegateCommand<object>((o) => {
                NexI2cOperators.Add(new NexI2cOperator() { SlaveAddr = 0x32, RegAddr = 0, Size = 8 });
            });
            Delete = new DelegateCommand<object>((o) => {
                var cmd = o as NexI2cOperator;
                if (cmd == null) return;

                NexI2cOperators.Remove(cmd);
            });
        }

        ObservableCollection<NexI2cOperator> _NexI2cOperators = new ObservableCollection<NexI2cOperator>()
        {
            new NexI2cOperator(){ SlaveAddr = 0x32, RegAddr = 0, Size = 8},
            new NexI2cOperator(){ SlaveAddr = 0x32, RegAddr = 0, Size = 1},
        };
        public ObservableCollection<NexI2cOperator> NexI2cOperators { get { return _NexI2cOperators; } set { _NexI2cOperators = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> WriteRead { get; set; }
        public DelegateCommand<object> Write { get; set; }

        public DelegateCommand<object> Add { get; set; }
        public DelegateCommand<object> Delete { get; set; }
    }
}
