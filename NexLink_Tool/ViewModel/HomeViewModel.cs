using Hexconverters;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
                        int outLen = -1;
                        Manager.nexLink.TransferData(rawData, rawData.Length, ref outLen);
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

            Receive = new DelegateCommand<object>(async (o) =>
            {
                try
                {
                    if (Manager.nexLink.IsConnected)
                    {
                        var rawData = new byte[1024];
                        int outLen = -1;
                        Manager.nexLink.ReceiveData(ref rawData, rawData.Length, ref outLen);
                        if (outLen > 0)
                        {
                            Log += $"[{DateTime.Now.ToString("HH:mm:ss.fff")}] Rx:\n";
                            Log += (Hexstring.ToString(rawData, outLen) + "\n");
                            if (rawData[0] > 127 || rawData[0] < 0x20)
                                Log += "\n";
                            else
                                Log += (Encoding.ASCII.GetString(rawData, 0, outLen) + "\n\n");
                        }
                    }
                    else
                        Manager.ShowNoti("Device not connected!");
                }
                catch (Exception)
                {
                }
                await Task.Delay(10);
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
