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
                        var rawdata = new byte[cmd.Size];
                        Task.Run(() =>
                        {
                            var ret = Manager.nexLink.ControlGetData((NEX_BREQ)cmd.Addr, rawdata);
                            cmd.Data = Hexstring.ToString(rawdata);
                            cmd.AsciiData = Encoding.Default.GetString(rawdata);
                        });
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
                        Task.Run(() =>
                        {
                            var ret = Manager.nexLink.ControlSetData((NEX_BREQ)cmd.Addr, Hexstring.GetBytes(cmd.Data));
                            if (ret < 0)
                                Manager.ShowNoti("Set data failed", Wpf.Ui.Controls.ControlAppearance.Caution);
                        });
                    }
                    catch (Exception e)
                    {
                        Manager.ShowNoti(e.Message, Wpf.Ui.Controls.ControlAppearance.Danger);
                    }
                }
                else
                    Manager.ShowNoti("Device not connected!");
            });
            Add = new DelegateCommand<object>((o) => {
                NexCommands.Add(new NexCommand() { Addr = 0x10, Size = 32 * 2, Data = "" });
            });
            Delete = new DelegateCommand<object>((o) => {
                var cmd = o as NexCommand;
                if (cmd == null) return;

                NexCommands.Remove(cmd);
            });
            SyncTime = new DelegateCommand<object>((o) => {
                if (Manager.nexLink.IsConnected)
                {
                    Manager.nexLink.SetTimestamp();
                }
                else
                    Manager.ShowNoti("Device not connected!");
            });
            GetName = new DelegateCommand<object>((o) => {
                if (Manager.nexLink.IsConnected)
                {
                    string name = string.Empty;
                    Manager.nexLink.GetNameDes(ref name);
                    Manager.ShowNoti(name);
                }
                else
                    Manager.ShowNoti("Device not connected!");
            });
            GetLog = new DelegateCommand<object>((o) => {
                if (Manager.nexLink.IsConnected)
                {
                    Task.Run(() =>
                    {
                        nex_log_des log_Des = new nex_log_des();
                        Manager.nexLink.GetLogDes(ref log_Des);
                        if(log_Des.size == 0)
                        {
                            Manager.ShowNoti("Device not log now!");
                            return;
                        }
                        nex_log_data log_Data = new nex_log_data();
                        Manager.nexLink.GetLogData(ref log_Data);
                        {
                            Manager.ShowNoti($"{Hexstring.ToString(log_Data.data, log_Data.len)}", $"{new DateTime(log_Data.timestamp * 10000).ToString("HH:mm:ss.fff")} - {log_Data.type}_{log_Data.dir}, Remaining {log_Des.size}", Wpf.Ui.Controls.ControlAppearance.Light, 5);
                        }
                    });
                }
                else
                    Manager.ShowNoti("Device not connected!");
            });
            AutoRead = new DelegateCommand<object>((o) => {
                var cmd = o as NexCommand;
                if (cmd == null) return;

                cmd.IsAutoRead = !cmd.IsAutoRead;
            });

            Task.Run(async () => {

                while (true)
                {
                    if (Manager.nexLink.IsConnected)
                    {
                        for (int i = 0; i < NexCommands.Count; i++)
                        {
                            var cmd = NexCommands[i];
                            if(cmd.IsAutoRead)
                            {
                                Read.Execute(cmd);
                                await Task.Delay(1);
                            }
                        }
                    }

                    await Task.Delay(1);
                }
            });
        }

        ObservableCollection<NexCommand> _NexCommands = new ObservableCollection<NexCommand>()
        {
            new NexCommand(){ Addr = 0x11, Size = 32*2, Data =""},
            new NexCommand(){ Addr = 0x10, Size = 32*2, Data =""},

        };
        public ObservableCollection<NexCommand> NexCommands { get { return _NexCommands; } set { _NexCommands = value; RaisePropertyChanged(); } }

        public DelegateCommand<object> Read { get; set; }
        public DelegateCommand<object> Write { get; set; }

        public DelegateCommand<object> Add { get; set; }
        public DelegateCommand<object> Delete { get; set; }
        public DelegateCommand<object> AutoRead { get; set; }

        public DelegateCommand<object> SyncTime { get; set; }
        public DelegateCommand<object> GetName { get; set; }
        public DelegateCommand<object> GetLog { get; set; }
    }
}
