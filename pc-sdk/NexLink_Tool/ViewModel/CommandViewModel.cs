using Hexconverters;
using Microsoft.Win32;
using NexLink;
using NexLink_Tool.Model;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace NexLink_Tool.ViewModel
{
    internal class CommandViewModel : BindableBase
    {
        internal CommandViewModel()
        {
            Request = new DelegateCommand<object>((o) => {
                var cmd = o as NexCommand;
                if (cmd == null) return;

                try
                {
                    var resp = Manager.dev.SendCommand(cmd.Cmd, Hexstring.GetBytes(cmd.Data));
                    var data = NexLinkManager.GetRespData(resp);

                    var info = "Request successfully";
                    if (data.Length > 0)
                    {
                        info += $"\r\n{Hexstring.ToString(data.ToArray())}";
                    }
                    Manager.ShowNoti(info, Wpf.Ui.Controls.ControlAppearance.Success);
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}");
                }
            });
            Add = new DelegateCommand<object>((o) => {
                NexCommands.Add(new NexCommand() { Cmd = NexLinkCmd.CmdPing});
            });
            Delete = new DelegateCommand<object>((o) => {
                var cmd = o as NexCommand;
                if (cmd == null) return;

                NexCommands.Remove(cmd);
            });
            Execute = new DelegateCommand<object>((o) =>
            {
                string parameter = (string)o;
                var dev = Manager.dev;
                try
                {
                    switch (parameter)
                    {
                        case "SYNC Time":
                            long offset = dev.SyncTimeMs();
                            Manager.ShowNoti($"Time offset(ms): {offset}");
                            break;
                        case "Up":
                            dev.SendCommand(NexLinkCmd.CmdKey, new byte[] { (byte)ConsoleKey.UpArrow });
                            break;
                        case "Down":
                            dev.SendCommand(NexLinkCmd.CmdKey, new byte[] { (byte)ConsoleKey.DownArrow });
                            break;
                        case "Enter":
                            dev.SendCommand(NexLinkCmd.CmdKey, new byte[] { (byte)ConsoleKey.RightArrow });
                            break;
                        case "Exit":
                            dev.SendCommand(NexLinkCmd.CmdKey, new byte[] { (byte)ConsoleKey.LeftArrow });
                            break;
                        default:
                            break;
                    }
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}");
                }
            });

        }
        ObservableCollection<NexCommand> _NexCommands = new ObservableCollection<NexCommand>()
        {

        };

        public Array NexLinkCmdValues { get; } =
            Enum.GetValues(typeof(NexLinkCmd));

        public ObservableCollection<NexCommand> NexCommands { get { return _NexCommands; } set { _NexCommands = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> Request { get; set; }
        public DelegateCommand<object> Execute { get; set; }
        public DelegateCommand<object> Add { get; set; }
        public DelegateCommand<object> Delete { get; set; }
    }
}
