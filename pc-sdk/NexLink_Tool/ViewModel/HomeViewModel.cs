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
        private StreamOverlay _streamOverlay;
        private StreamOverlay _cameraOverlay;

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
                // The camera transfer is intentionally left running while this page is off-screen;
                // it stops only via its toggle, via the stream window closing, or on disconnect.
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
                try
                {
                    if (isChecked == true)
                    {
                        StopStreamOverlay();
                        StopCameraOverlay();
                        Manager.dev?.SendCommand(NexLinkCmd.CmdFrameGet, [0xFF]);
                    }
                    else
                    {
                        Manager.dev?.SendCommand(NexLinkCmd.CmdFrameGet, [0]);
                        await Task.Delay(100);
                        DisplayImage = null;
                    }

                }
                catch (Exception)
                {
                }
            });
            ToggleStreamCommand = new DelegateCommand<bool?>(async (isChecked) =>
            {
                if (isChecked != true)
                {
                    StopStreamOverlay();
                    return;
                }

                if (Manager.dev == null)
                {
                    Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                    IsStreaming = false;
                    return;
                }

                try
                {
                    // Only one capture mode may run at a time: video frames and sync still-image
                    // uploads would interleave on the same link.
                    await StopSyncAsync();
                    StopCameraOverlay();

                    var info = Manager.dev.GetDisplayInfo();
                    if (info.DisplayCount == 0)
                    {
                        Manager.ShowNoti("Device has no display", ControlAppearance.Caution);
                        IsStreaming = false;
                        return;
                    }
                    if (info.Displays[0].Bpp == ImageBppConverter.TargetPixelFormat.Dual2ColorGray8)
                    {
                        Manager.ShowNoti("Device has not support", ControlAppearance.Caution);
                        IsStreaming = false;
                        return;
                    }

                    Manager.AppendLog("[STREAM] Starting video stream...", LogLevel.Info);

                    _streamOverlay = new StreamOverlay(Manager.dev, info.Displays[0]);
                    _streamOverlay.Closed += StreamOverlay_Closed;
                    _streamOverlay.Show();
                }
                catch (Exception ex)
                {
                    IsStreaming = false;
                    Manager.ShowNoti($"Stream failed: {ex.Message}", ControlAppearance.Danger);
                }
            });
            ToggleCameraCommand = new DelegateCommand<bool?>(async (isChecked) =>
            {
                if (isChecked != true)
                {
                    StopCameraOverlay();
                    return;
                }

                if (Manager.dev == null)
                {
                    Manager.ShowNoti($"Not connected any device", ControlAppearance.Secondary);
                    IsCameraStreaming = false;
                    return;
                }

                try
                {
                    await StopSyncAsync();
                    StopStreamOverlay();

                    var info = Manager.dev.GetDisplayInfo();
                    if (info.DisplayCount == 0)
                    {
                        Manager.ShowNoti("Device has no display", ControlAppearance.Caution);
                        IsCameraStreaming = false;
                        return;
                    }
                    if (info.Displays[0].Bpp == ImageBppConverter.TargetPixelFormat.Dual2ColorGray8)
                    {
                        Manager.ShowNoti("Device has not support", ControlAppearance.Caution);
                        IsStreaming = false;
                        return;
                    }

                    Manager.AppendLog("[CAMERA] Starting camera transfer...", LogLevel.Info);
                    _cameraOverlay = new StreamOverlay(Manager.dev, info.Displays[0], true);
                    _cameraOverlay.Closed += CameraOverlay_Closed;
                    _cameraOverlay.StartBackgroundCapture();
                }
                catch (Exception ex)
                {
                    IsCameraStreaming = false;
                    Manager.ShowNoti($"Camera transfer failed: {ex.Message}", ControlAppearance.Danger);
                }
            });
        }

        private async Task StopSyncAsync()
        {
            if (!IsSyncing)
                return;

            Manager.dev?.SendAsync(NexLinkCmd.CmdFrameGet, [0]);
            IsSyncing = false;
            await Task.Delay(100);
            DisplayImage = null;
        }

        /// <summary>
        /// 关掉"同步取图"模式：告诉设备别再上传图像，并清掉界面上的那一帧。
        /// 给"断开设备 / 退出前收尾"用（UI 线程上调用，不阻塞，也不需要 await）。
        /// </summary>
        public void DisableSyncMode()
        {
            if (!IsSyncing)
                return;

            Manager.dev?.SendAsync(NexLinkCmd.CmdFrameGet, [0]);
            IsSyncing = false;
            DisplayImage = null;
        }

        private void StopStreamOverlay()
        {
            var overlay = _streamOverlay;
            _streamOverlay = null;
            IsStreaming = false;
            overlay?.Stop();
        }

        /// <summary>
        /// 退出 / 断开设备前调用：停掉视频流和摄像头流，并等它们的发送线程收尾。
        /// 收尾里会往设备发一张全黑图（见 StreamOverlay.SendLoop），等它发完，
        /// 才能保证设备最后收到的是黑屏而不是某一帧画面。
        /// </summary>
        public void StopAllStreamsAndWait(int timeoutMs)
        {
            var stream = _streamOverlay;
            var camera = _cameraOverlay;

            StopStreamOverlay();
            StopCameraOverlay();

            stream?.WaitStopped(timeoutMs);
            camera?.WaitStopped(timeoutMs);
        }

        private void StopCameraOverlay()
        {
            var overlay = _cameraOverlay;
            _cameraOverlay = null;
            IsCameraStreaming = false;
            overlay?.Stop();
        }

        private void StreamOverlay_Closed(object sender, EventArgs e)
        {
            _streamOverlay = null;
            IsStreaming = false;
            Manager.AppendLog("[STREAM] Stopped video stream...", LogLevel.Info);
        }

        private void CameraOverlay_Closed(object sender, EventArgs e)
        {
            _cameraOverlay = null;
            IsCameraStreaming = false;
            Manager.AppendLog("[CAMERA] Camera transfer stopped.", LogLevel.Info);
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

        private bool _isStreaming;

        public bool IsStreaming
        {
            get => _isStreaming;
            set
            {
                _isStreaming = value;
                RaisePropertyChanged();
            }
        }

        private bool _isCameraStreaming;

        public bool IsCameraStreaming
        {
            get => _isCameraStreaming;
            set
            {
                _isCameraStreaming = value;
                RaisePropertyChanged();
            }
        }

        public DelegateCommand<object> LoadedCommand { get; set; }
        public DelegateCommand<object> UnloadedCommand { get; set; }
        public DelegateCommand<object> ExportImageCommand { get; set; }
        public DelegateCommand<bool?> ToggleSyncCommand { get; set; }
        public DelegateCommand<bool?> ToggleStreamCommand { get; set; }
        public DelegateCommand<bool?> ToggleCameraCommand { get; set; }
    }
}
