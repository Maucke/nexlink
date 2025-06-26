using Hexconverters;
using NexLink_Net;
using NexLink_NET;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace NexLink_Tool.ViewModel
{
    internal class HomeViewModel : BindableBase
    {
        internal HomeViewModel()
        {
            Transfer = new DelegateCommand<object>((o) => {
                if (Manager.nexLink.IsConnected)
                {
                    try
                    {
                        var rawData = Hexstring.GetBytes(Val);
                        Thread thread = new Thread(() =>
                        {
                            var ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(100);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(200);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                            Thread.Sleep(10);
                            ret = NexLink_Peripheral.UART_WriteBytes(0, rawData, (short)rawData.Length);
                        })
                        { IsBackground = true };
                        thread.Start();
                        Log += $"[{DateTime.Now.ToString("HH:mm:ss.fff")}] Tx:\n";
                        Log += (Hexstring.ToString(rawData) + "\n\n");
                    }
                    catch (Exception e)
                    {
                        Manager.ShowNoti(e.Message, Wpf.Ui.Controls.ControlAppearance.Danger);
                    }
                }
                else
                    Manager.ShowNoti("Device not connected!");
            });

            Unloaded = new DelegateCommand<object>((o) => { });

            Task.Run(async () =>
            {
                while(true)
                {
                    try
                    {
                        if (Manager.nexLink.IsConnected)
                        {
                            var rawData = new byte[1024];
                            var outLen = NexLink_Peripheral.UART_ReadBytes(0, rawData, 10);
                            if (outLen > 0)
                            {
                                Log += $"[{DateTime.Now.ToString("HH:mm:ss.fff")}] Rx:\n";
                                Log += (Hexstring.ToString(rawData, outLen) + "\n");
                            }
                        }
                        else
                            Manager.ShowNoti("Device not connected!");
                    }
                    catch (Exception)
                    {
                    }
                    await Task.Delay(10);
                }
            });
        }

        String _Log;
        public String Log { get { return _Log; } set { _Log = value; RaisePropertyChanged(); } }

        String _Val = "11 22 33 44 ";
        public String Val { get { return _Val; } set { _Val = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> Transfer { get; set; }
        public DelegateCommand<object> Receive { get; set; }

        public DelegateCommand<object> Unloaded { get; set; }
    }
}
