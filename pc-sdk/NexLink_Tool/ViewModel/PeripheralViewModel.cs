using Hexconverters;
using NexLink;
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
using System.Windows.Markup;
using Wpf.Ui.Controls;

namespace NexLink_Tool.ViewModel
{
    internal class PeripheralViewModel : BindableBase
    {
        internal PeripheralViewModel()
        {
            I2cWriteRead = new DelegateCommand<object>((o) => {
                var dev = Manager.dev;
                var ch = Convert.ToByte(I2cChn.Replace("CH", ""));
                var operat = o as NexI2cOperator;
                byte[] write = Hexstring.GetBytes(operat.RegAddr + " " + operat.Data);

                try
                {
                    var result = dev.I2cTransfer(
                        busId: ch,
                        slaveAddr: operat.SlaveAddr,
                        writeData: write,
                        readLength: operat.Size,
                        sendStop: true,
                        repeatedStart: true,
                        timeoutMs: 500);

                    Manager.ShowNoti($"WriteRead I2c result: {Hexstring.ToString(result)}", ControlAppearance.Success);
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                }
            });

            I2cWrite = new DelegateCommand<object>((o) => {
                var dev = Manager.dev;
                var ch = Convert.ToByte(I2cChn.Replace("CH", ""));
                var operat = o as NexI2cOperator;
                byte[] write = Hexstring.GetBytes(operat.RegAddr + " " + operat.Data);

                try
                {
                    var result = dev.I2cTransfer(
                        busId: ch,
                        slaveAddr: operat.SlaveAddr,
                        writeData: write,
                        readLength: 0,
                        sendStop: true,
                        repeatedStart: true,
                        timeoutMs: 500);

                    Manager.ShowNoti($"Write I2c successfully", ControlAppearance.Success);
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                }
            });

            I2cAdd = new DelegateCommand<object>((o) => {
                NexI2cOperators.Add(new NexI2cOperator() { SlaveAddr = 0x32, RegAddr = 0, Size = 8 });
            });

            I2cDelete = new DelegateCommand<object>((o) => {
                var cmd = o as NexI2cOperator;
                if (cmd == null) return;

                NexI2cOperators.Remove(cmd);
            });

            I2cInit = new DelegateCommand<object>((o) => {
                var dev = Manager.dev;
                var ch = Convert.ToByte(I2cChn.Replace("CH", ""));
                try
                {
                    var clock = I2cClock;
                    Manager.dev.ConfigureI2c(ch, clock);
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                }
            });

            Test = new DelegateCommand<object>((o) => {

            });

            UartInit = new DelegateCommand<object>((o) => {
                var dev = Manager.dev;
                var ch = Convert.ToByte(UartChn.Replace("CH", ""));
                try
                {
                    var baudrate = UartBaudRate;
                    Manager.dev.ConfigureUart(ch, baudrate);
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                }
            });

            UartSend = new DelegateCommand<object>((o) => {
                var dev = Manager.dev;
                var ch = Convert.ToByte(UartChn.Replace("CH", ""));
                try
                {
                    var data = Hexstring.GetBytes(UartData);
                    Manager.dev.UartWrite(ch, data);

                    if (data.Length > 0)
                    {
                        UartLog += $"[{DateTime.Now.ToString("HH:mm:ss.fff")}-{ch}] TX: {Hexstring.ToString([.. data])}\r\n";
                    }
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
                }
            });
        }

        ObservableCollection<NexI2cOperator> _NexI2cOperators = new ObservableCollection<NexI2cOperator>()
        {
            new NexI2cOperator(){ SlaveAddr = 0x32, RegAddr = 0, Size = 8},
            new NexI2cOperator(){ SlaveAddr = 0x32, RegAddr = 0, Size = 1},
        };
        public ObservableCollection<NexI2cOperator> NexI2cOperators { get { return _NexI2cOperators; } set { _NexI2cOperators = value; RaisePropertyChanged(); } }

        public List<string> UartChns { get; set; } = new List<string> { "CH0", "CH1" };

        string _UartChn = "CH0";
        public string UartChn { get { return _UartChn; } set { _UartChn = value; RaisePropertyChanged(); } }

        public List<string> I2cChns { get; set; } = new List<string> { "CH0", "CH1" };

        string _I2cChn = "CH0";
        public string I2cChn { get { return _I2cChn; } set { _I2cChn = value; RaisePropertyChanged(); } }
        uint _I2cClock = 400000;
        public uint I2cClock { get { return _I2cClock; } set { _I2cClock = value; RaisePropertyChanged(); } }

        uint _UartBaudRate = 115200;
        public uint UartBaudRate { get { return _UartBaudRate; } set { _UartBaudRate = value; RaisePropertyChanged(); } }

        string _UartLog;
        public string UartLog { get { return _UartLog; } set { _UartLog = value; RaisePropertyChanged(); } }

        string _UartData = "11 22 33 44";
        public string UartData { get { return _UartData; } set { _UartData = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> I2cWriteRead { get; set; }
        public DelegateCommand<object> I2cWrite { get; set; }

        public DelegateCommand<object> I2cAdd { get; set; }
        public DelegateCommand<object> I2cDelete { get; set; }
        public DelegateCommand<object> I2cInit { get; set; }
        public DelegateCommand<object> Test { get; set; }

        public DelegateCommand<object> UartAdd { get; set; }
        public DelegateCommand<object> UartSend { get; set; }
        public DelegateCommand<object> UartInit { get; set; }
        public DelegateCommand<object> UartTest { get; set; }
    }
}
