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
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
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

                    if (data.Length > 0)
                    {
                        var info = $"RSP: {Hexstring.ToString(data.ToArray())}";
                        Manager.ShowNoti(info, ControlAppearance.Success);
                    }
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
                string parameter = (string)o;
                var dev = Manager.dev;
                try
                {
                    switch (parameter)
                    {
                        case "SYNC Time":
                            if (dev == null)
                            {
                                Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                                return;
                            }
                            long offset = dev.SyncTimeMs();
                            Manager.ShowNoti($"Time offset(ms): {offset}");
                            break;
                        case "GetDisplayInfo":
                            if (dev == null)
                            {
                                Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                                return;
                            }
                            {
                                var info = dev.GetDisplayInfo();
                                Manager.ShowNoti($"{info}");
                            }
                            break;
                        case "Show Picture":
                            if (dev == null)
                            {
                                Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                                return;
                            }
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
                                        await Task.Delay(100);
                                        dev.SendFrame(image, firstscreen.Bpp);

                                        //Manager.ShowNoti("Image sent successfully!");
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
                            catch (Exception)
                            { 
                                
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
                        case "Update Boot":
                            if (dev == null)
                            {
                                Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                                return;
                            }
                            {
                                var dlg = new OpenFileDialog
                                {
                                    Title = "Select bootloader image",
                                    Filter = "Firmware (*.bin)|*.bin|All files (*.*)|*.*",
                                    Multiselect = false
                                };

                                if (dlg.ShowDialog() != true)
                                    return;

                                byte[] raw = File.ReadAllBytes(dlg.FileName);
                                int hdr = FirmwareImage.HeaderLength(raw);
                                byte[] payload = new byte[raw.Length - hdr];
                                Array.Copy(raw, hdr, payload, 0, payload.Length);

                                string header = hdr > 0
                                    ? Encoding.ASCII.GetString(raw, 0, hdr).Trim()
                                    : "(no header)";

                                string error;
                                if (!FirmwareImage.ValidateBoot(payload, out error))
                                {
                                    Manager.ShowNoti(error, ControlAppearance.Danger);
                                    return;
                                }

                                Manager.AppendLog(
                                    $"[FW] {Path.GetFileName(dlg.FileName)} | {header} | {payload.Length} bytes",
                                    LogLevel.Info);

                                var ok = await Manager.ShowDialog(
                                    $"Write bootloader to 0x08000000?\n\n" +
                                    $"{header}\n{payload.Length} bytes\n\n" +
                                    "Do not unplug the device until it restarts.",
                                    "Update Bootloader");

                                if (ok != ContentDialogResult.Primary)
                                    return;

                                try
                                {
                                    int lastPct = -10;

                                    dev.UpdateFirmware(payload, 1, p =>
                                    {
                                        int pct = (int)(p * 100);

                                        if (pct >= lastPct + 10 || pct == 100)
                                        {
                                            lastPct = pct;
                                            Manager.AppendLog($"[FW] {pct}%", LogLevel.Info);
                                        }
                                    });

                                    Manager.ShowNoti(
                                        "Bootloader updated, device is restarting",
                                        ControlAppearance.Success);

                                    await Task.Delay(2000);

                                    var vm = Manager.settingViewModel;
                                    var ndev = vm.NexDevices.FirstOrDefault(x => x.IsConnect);
                                    if (ndev == null) ndev = vm.NexDevices.FirstOrDefault();
                                    if (ndev != null)
                                    {
                                        ndev.IsConnect = true;
                                        vm.Control.Execute(ndev);
                                    }
                                }
                                catch (Exception ex)
                                {
                                    Manager.ShowNoti(
                                        $"Update failed: {ex.Message}. Do not power off - retry the update.",
                                        ControlAppearance.Danger);
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

            new NexCommand("Bootloader",NexLinkCmd.CmdBootloader),

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

    /* Bootloader .bin handling: addheader.bat puts an "appheader vX.xx ..."
     * line in front of the raw bin, so strip it before sending. */
    public static class FirmwareImage
    {
        public static int HeaderLength(byte[] data)
        {
            if (data == null || data.Length < 10)
                return 0;

            if (Encoding.ASCII.GetString(data, 0, 9) != "appheader")
                return 0;

            for (int i = 9; i < data.Length && i < 64; i++)
                if (data[i] == (byte)'\n')
                    return i + 1;

            return 32;
        }

        public static bool ValidateBoot(byte[] payload, out string error)
        {
            error = null;

            if (payload == null || payload.Length < 1024)
            {
                error = "Image is too small";
                return false;
            }

            if (payload.Length > 0xC000)
            {
                error = $"Bootloader image is {payload.Length} bytes, max 48 KB";
                return false;
            }

            uint sp = BitConverter.ToUInt32(payload, 0);
            uint pc = BitConverter.ToUInt32(payload, 4);

            if (sp < 0x20000000 || sp > 0x20010000)
            {
                error = $"Bad initial SP 0x{sp:X8}";
                return false;
            }

            /* reset vector must point into the bootloader region */
            if (pc < 0x08000000 || pc >= 0x0800C000 || (pc & 1) == 0)
            {
                error = $"Bad reset vector 0x{pc:X8} - is this a bootloader bin?";
                return false;
            }

            return true;
        }
    }
}
