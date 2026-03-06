using GongSolutions.Wpf.DragDrop;
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
    public class NexCommandDropHandler : DefaultDropHandler
    {
        public override void DragOver(IDropInfo dropInfo)
        {
            if (dropInfo is DropInfo typedDropInfo)
            {
                if (typedDropInfo.Data is NexCommand cmd)
                {
                    typedDropInfo.Data = new NexCommand(cmd.Name, cmd.Cmd, cmd.Data);
                    base.DragOver(typedDropInfo);
                }
            }
        }
    }

    internal class CommandViewModel : BindableBase
    {
        public NexCommandDropHandler DropHandler { get; } = new NexCommandDropHandler();
        internal CommandViewModel()
        {
            Request = new DelegateCommand<object>((o) => {
                if (Manager.dev == null)
                {
                    Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                    return;
                }
                var cmd = o as NexCommand;
                if (cmd == null) return;

                try
                {
                    var resp = Manager.dev.SendCommand(cmd.Cmd, Hexstring.GetBytes(cmd.Data));
                    var data = NexLinkManager.GetRespData(resp);

                    var info = $"{cmd.Name} Request successfully";
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
                NexCommands.Add(new NexCommand($"{NexLinkCmd.CmdPing}", NexLinkCmd.CmdPing));
            });
            Delete = new DelegateCommand<object>((o) => {
                var cmd = o as NexCommand;
                if (cmd == null) return;

                if (NexCommands.Contains(cmd))
                    NexCommands.Remove(cmd);
                else if (CustomNexCommands.Contains(cmd))
                    CustomNexCommands.Remove(cmd);
            });
            Execute = new DelegateCommand<object>(async (o) =>
            {
                if (Manager.dev == null)
                {
                    Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                    return;
                }
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
                                            throw new Exception("No valid screen");
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
                        case "Reset":
                            try
                            {
                                Manager.dev.SendAsync(NexLinkCmd.CmdHwReset);
                            }
                            finally
                            {
                                await Task.Delay(1000);
                                {
                                    var vm = Manager.settingViewModel;
                                    var ndev = vm.NexDevices.FirstOrDefault(x => x.IsConnect);
                                    if (ndev == null) ndev = vm.NexDevices.FirstOrDefault();
                                    if (ndev != null)
                                    {
                                        ndev.IsConnect = true;
                                        vm.Control.Execute(ndev);
                                    }
                                }
                            }
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
        public Array NexLinkCmdValues { get; } =
            Enum.GetValues(typeof(NexLinkCmd));

        ObservableCollection<NexCommand> _NexCommands = new ObservableCollection<NexCommand>()
        {

        };

        public ObservableCollection<NexCommand> NexCommands { get { return _NexCommands; } set { _NexCommands = value; RaisePropertyChanged(); } }

        ObservableCollection<NexCommand> _CustomNexCommands = new ObservableCollection<NexCommand>()
             {
            new NexCommand("SYNC Time", NexLinkCmd.CmdSyncTime),

            new NexCommand("Reset",NexLinkCmd.CmdHwReset),

            new NexCommand("ScreenShot", NexLinkCmd.CmdFrameGet,Hexstring.ToString( [1])),

            new NexCommand("Up", NexLinkCmd.CmdKey,Hexstring.ToString(  [(byte)ConsoleKey.UpArrow])),

            new NexCommand("Down",NexLinkCmd.CmdKey ,Hexstring.ToString(   [(byte)ConsoleKey.DownArrow])),

            new NexCommand("Enter", NexLinkCmd.CmdKey,Hexstring.ToString( [(byte)ConsoleKey.RightArrow])),

            new NexCommand("Exit", NexLinkCmd.CmdKey, Hexstring.ToString( new byte[] { (byte)ConsoleKey.LeftArrow })),
             };

        public ObservableCollection<NexCommand> CustomNexCommands { get { return _CustomNexCommands; } set { _CustomNexCommands = value; RaisePropertyChanged(); } }
        public DelegateCommand<object> Request { get; set; }
        public DelegateCommand<object> Execute { get; set; }
        public DelegateCommand<object> Add { get; set; }
        public DelegateCommand<object> Delete { get; set; }
    }
}
