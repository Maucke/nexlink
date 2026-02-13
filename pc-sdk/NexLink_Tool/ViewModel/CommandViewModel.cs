using Hexconverters;
using ImageBppConverter;
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
using Wpf.Ui.Controls;

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
                    Manager.ShowNoti(info, ControlAppearance.Success);
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
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
                        case "Reset":
                            dev.SendCommand(NexLinkCmd.CmdHwReset, null);
                            break;
                        case "GetDisplayInfo":
                            {
                                var info = dev.GetDisplayInfo();
                                Manager.ShowNoti($"{info}");
                            }
                            break;
                        case "Show Picture":
                            {
                                OpenFileDialog dlg = new OpenFileDialog
                                {
                                    Title = "Select Image",
                                    Filter = "Image Files|*.png;*.jpg;*.bmp",
                                    Multiselect = false
                                };

                                if (dlg.ShowDialog() == true)
                                {
                                    try
                                    {
                                        var info = dev.GetDisplayInfo();
                                        if (info.DisplayCount == 0)
                                            throw new Exception("无屏幕可显示");
                                        var firstscreen = info.Displays.First();

                                        Bitmap bmp = new Bitmap(dlg.FileName);
                                        Bitmap resized = new Bitmap(bmp, new Size(firstscreen.Width, firstscreen.Height));

                                        var image = ImageBppConverter.ImageConverter
                                            .Convert(resized, firstscreen.Bpp);

                                        dev.SendFrame(image, firstscreen.Bpp);

                                        Manager.ShowNoti("Image sent successfully!");
                                    }
                                    catch (Exception ex)
                                    {
                                        Manager.ShowNoti(ex.Message);
                                    }
                                }
                            }
                            break;
                        case "ScreenShot":
                            dev.SendCommand(NexLinkCmd.CmdFrameGet, [1]);
                            break;
                        case "Up":
                            dev.SendCommand(NexLinkCmd.CmdKey, [(byte)ConsoleKey.UpArrow]);
                            break;
                        case "Down":
                            dev.SendCommand(NexLinkCmd.CmdKey, [(byte)ConsoleKey.DownArrow]);
                            break;
                        case "Enter":
                            dev.SendCommand(NexLinkCmd.CmdKey, [(byte)ConsoleKey.RightArrow]);
                            break;
                        case "Exit":
                            dev.SendCommand(NexLinkCmd.CmdKey, [(byte)ConsoleKey.LeftArrow]);
                            break;
                        default:
                            break;
                    }
                }
                catch (Exception e)
                {
                    Manager.ShowNoti($"{e.Message}", ControlAppearance.Caution);
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
