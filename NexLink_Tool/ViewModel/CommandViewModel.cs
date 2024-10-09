using Hexconverters;
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
    internal class CommandViewModel : BindableBase
    {
        internal CommandViewModel()
        {
            Read = new DelegateCommand<object>((o) => {
                var cmd = o as NexCommand;
                if (cmd == null) return;
                if (Manager.nexLink.IsConnected)
                {
                    try
                    {
                        var rawdata = new byte[128];
                        var ret = Manager.nexLink.ControlGetData((NEX_BREQ)cmd.Addr, rawdata);
                        cmd.Data = Hexstring.ToString(rawdata);
                    }
                    catch (Exception e)
                    {
                        Manager.ShowNoti(e.Message, Wpf.Ui.Controls.ControlAppearance.Danger);
                    }
                }
                else
                    Manager.ShowNoti("Device not connected!");
            });
            Write = new DelegateCommand<object>((o) => {
                var cmd = o as NexCommand;
                if (cmd == null) return;
                if (Manager.nexLink.IsConnected)
                {
                    try
                    {
                        var rawdata = new byte[128];
                        var ret = Manager.nexLink.ControlSetData((NEX_BREQ)cmd.Addr, Hexstring.GetBytes(cmd.Data));
                    }
                    catch (Exception e)
                    {
                        Manager.ShowNoti(e.Message, Wpf.Ui.Controls.ControlAppearance.Danger);
                    }
                }
                else
                    Manager.ShowNoti("Device not connected!");
            });
        }

        ObservableCollection<NexCommand> _NexCommands = new ObservableCollection<NexCommand>()
        {
            new NexCommand(){ Addr = 7,Data=""}
        };
        public ObservableCollection<NexCommand> NexCommands { get { return _NexCommands; } set { _NexCommands = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> Read { get; set; }
        public DelegateCommand<object> Write { get; set; }
    }
}
