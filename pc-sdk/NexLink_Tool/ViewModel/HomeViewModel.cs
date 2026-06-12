using Hexconverters;
using NexLink;
using NexLink_Tool.Model;
using NexLink_Tool.Page;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using Wpf.Ui.Controls;

namespace NexLink_Tool.ViewModel
{
    internal class HomeViewModel : BindableBase
    {
        internal HomeViewModel()
        {
            LoadedCommand = new DelegateCommand<object>((o) => {
                if (Manager.dev != null && Manager.homeViewModel.IsSyncing)
                {
                    Task.Run(() =>
                    {
                        try
                        {
                            var info = Manager.dev?.GetDisplayInfo();
                            if (info.DisplayCount > 0)
                                Manager.dev?.SendCommand(NexLinkCmd.CmdFrameGet, [0xFF]);
                        }
                        catch (Exception)
                        {
                        }
                    });
                }
            });
            UnloadedCommand = new DelegateCommand<object>((o) => {
                if (Manager.dev != null && Manager.homeViewModel.IsSyncing)
                {
                    Task.Run(async () =>
                    {
                        try
                        {
                            var info = Manager.dev?.GetDisplayInfo();
                            if (info.DisplayCount > 0)
                                Manager.dev?.SendCommand(NexLinkCmd.CmdFrameGet, [0]);
                            await Task.Delay(100);
                            DisplayImage = null;
                        }
                        catch (Exception)
                        {
                        }
                    });
                }
            });
            ExportImageCommand = new DelegateCommand<object>((o) => {
                if (o is not BitmapSource bitmap)
                    return;

                try
                {
                    // 程序运行目录
                    string baseDir = AppDomain.CurrentDomain.BaseDirectory;

                    // ScreenShot 文件夹
                    string screenshotDir = Path.Combine(baseDir, "ScreenShot");

                    // 如果不存在就创建
                    if (!Directory.Exists(screenshotDir))
                        Directory.CreateDirectory(screenshotDir);

                    // 生成时间戳文件名
                    string fileName = $"ScreenShot_{DateTime.Now:yyyyMMdd_HHmmss}.png";

                    string fullPath = Path.Combine(screenshotDir, fileName);

                    // 保存 PNG
                    var encoder = new PngBitmapEncoder();
                    encoder.Frames.Add(BitmapFrame.Create(bitmap));

                    using var stream = new FileStream(fullPath, FileMode.Create);
                    encoder.Save(stream);
                    Manager.AppendLog("[UI] " + "ScreenShot successfully, " + fullPath, LogLevel.Info);
                }
                catch (Exception ex)
                {
                    Manager.AppendLog("[UI] " + "ScreenShot failed, " + ex.Message, LogLevel.Error);
                }
            });
            ToggleSyncCommand = new DelegateCommand<bool?>(async (isChecked) => {

                if(Manager.dev == null)
                {
                    Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                    IsSyncing = !IsSyncing;
                    return;
                }

                if (isChecked == true)
                {
                    Manager.dev?.SendCommand(NexLinkCmd.CmdFrameGet, [0xFF]);
                }
                else
                {
                    Manager.dev?.SendCommand(NexLinkCmd.CmdFrameGet, [0]);
                    await Task.Delay(100);
                    DisplayImage = null;
                }
            });
            StartStreamCommand = new DelegateCommand<object>(async (o) => {
                if (Manager.dev == null)
                {
                    Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                    return;
                }

                try
                {
                    var info = Manager.dev.GetDisplayInfo();
                    if (info.DisplayCount == 0)
                    {
                        Manager.ShowNoti("Device has no display", ControlAppearance.Caution);
                        return;
                    }

                    Manager.AppendLog("[STREAM] Starting video stream...", LogLevel.Info);

                    var overlay = new StreamOverlay(Manager.dev, info.Displays[0]);
                    overlay.Show();
                }
                catch (Exception ex)
                {
                    Manager.ShowNoti($"Stream failed: {ex.Message}", ControlAppearance.Danger);
                }
            });
        }
        public ObservableCollection<LogItem> Logs { get; } = new();

        private ImageSource _displayImage;
        public ImageSource DisplayImage
        {
            get => _displayImage;
            set
            {
                _displayImage = value;
                RaisePropertyChanged();
            }
        }
        private bool _isSyncing = true;

        public bool IsSyncing
        {
            get => _isSyncing;
            set
            {
                _isSyncing = value;
                RaisePropertyChanged();
            }
        }

        public DelegateCommand<object> LoadedCommand { get; set; }
        public DelegateCommand<object> UnloadedCommand { get; set; }
        public DelegateCommand<object> ExportImageCommand { get; set; }
        public DelegateCommand<bool?> ToggleSyncCommand { get; set; }
        public DelegateCommand<object> StartStreamCommand { get; set; }
    }
}
