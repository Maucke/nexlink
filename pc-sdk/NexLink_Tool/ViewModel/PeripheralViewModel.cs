using Hexconverters;
using NexLink_Tool.Model;
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
    internal class PeripheralViewModel : BindableBase
    {
        internal PeripheralViewModel()
        {
            WriteRead = new DelegateCommand<object>((o) => {
            });

            Write = new DelegateCommand<object>((o) => {
            });

            Add = new DelegateCommand<object>((o) => {
                NexI2cOperators.Add(new NexI2cOperator() { SlaveAddr = 0x32, RegAddr = 0, Size = 8 });
            });
            Delete = new DelegateCommand<object>((o) => {
                var cmd = o as NexI2cOperator;
                if (cmd == null) return;

                NexI2cOperators.Remove(cmd);
            });

            Init = new DelegateCommand<object>((o) => {
            });

            Test = new DelegateCommand<object>((o) => {

#if true
                var rawData = Hexstring.GetBytes("11 22 33 44 55");
                int outLen = -1;
                //Debug.WriteLine($"{Manager.nexLink.TransferData(NexLinkUser.EP2ADDR, rawData, rawData.Length, out outLen)}");
                //Debug.WriteLine($"{Manager.nexLink.TransferData(NexLinkUser.EP3ADDR, rawData, rawData.Length, out outLen)}");
                //Debug.WriteLine($"{Manager.nexLink.TransferData(4, rawData, rawData.Length, out outLen)}");
                return;
#endif

                //var readBytes = new byte[64];
                //nex_i2c_request i2cRequest = new nex_i2c_request();
                //Manager.nexLink.UartWriteRead(i2cRequest, ref readBytes);
                //return;
                if (!TestTaskNeedQuit)
                {
                    TestTaskNeedQuit = true;
                    Manager.ShowNoti("Test stop");
                    return;
                }
                var cmd = NexI2cOperators.FirstOrDefault();
                int fps = 0;
                if (cmd == null) return; 
                TestTaskNeedQuit = false;
                Task.Run(async () => {
                    while (!TestTaskNeedQuit)
                    {
                        await Task.Delay(1000);
                        Debug.WriteLine($"fps:{fps}");
                        // Manager.ShowNoti($"fps:{fps}");
                        fps = 0;
                    }
                });
                Task.Run(async () => {
                    while (!TestTaskNeedQuit)
                    {
                        WriteRead.Execute(cmd); fps++;
                        await Task.Delay(0);
                    }
                });
                Manager.ShowNoti("Test start");
            });

            UartTest = new DelegateCommand<object>((o) => {

                if (!UartTestTaskNeedQuit)
                {
                    UartTestTaskNeedQuit = true;
                    Manager.ShowNoti("Test stop");
                    return;
                }
                int fps = 0;
                UartTestTaskNeedQuit = false;
                Task.Run(async () => {
                    while (!UartTestTaskNeedQuit)
                    {
                        await Task.Delay(1000);
                        Debug.WriteLine($"fps:{fps}");
                        // Manager.ShowNoti($"fps:{fps}");
                        fps = 0;
                    }
                });
            });
        }
        bool TestTaskNeedQuit = true;
        bool UartTestTaskNeedQuit = true;

        ObservableCollection<NexI2cOperator> _NexI2cOperators = new ObservableCollection<NexI2cOperator>()
        {
            new NexI2cOperator(){ SlaveAddr = 0x32, RegAddr = 0, Size = 8},
            new NexI2cOperator(){ SlaveAddr = 0x32, RegAddr = 0, Size = 1},
        };
        public ObservableCollection<NexI2cOperator> NexI2cOperators { get { return _NexI2cOperators; } set { _NexI2cOperators = value; RaisePropertyChanged(); } }

        uint _BaudRate = 400000;
        public uint BaudRate { get { return _BaudRate; } set { _BaudRate = value; RaisePropertyChanged(); } }

        uint _UartBaudRate = 115200;
        public uint UartBaudRate { get { return _UartBaudRate; } set { _UartBaudRate = value; RaisePropertyChanged(); } }

        string _UartLog;
        public string UartLog { get { return _UartLog; } set { _UartLog = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> WriteRead { get; set; }
        public DelegateCommand<object> Write { get; set; }

        public DelegateCommand<object> Add { get; set; }
        public DelegateCommand<object> Delete { get; set; }
        public DelegateCommand<object> Init { get; set; }
        public DelegateCommand<object> Test { get; set; }

        public DelegateCommand<object> UartAdd { get; set; }
        public DelegateCommand<object> UartInit { get; set; }
        public DelegateCommand<object> UartTest { get; set; }
    }
}
