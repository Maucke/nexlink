using AxWMPLib;
using MahApps.Metro.Controls;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NexLink_NET;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;

namespace NexLink_MediaPlayer.ViewModel
{
    public class Configuration
    {
        public double Brightness { get; set; }
        public string Url { get; set; }
        public double Height { get; set; }

        public bool Save()
        {
            JObject Jmsg = new JObject();
            Jmsg.Add("config", JToken.FromObject(this));
            string filePath = "config.json";
            File.WriteAllText(filePath, Jmsg.ToString());
            return true;
        }

        public bool Load()
        {
            try
            {
                string filePath = "config.json";
                string json = File.ReadAllText(filePath, Encoding.UTF8);
                JObject jball = (JObject)JsonConvert.DeserializeObject(json);
                Configuration config = JsonConvert.DeserializeObject<Configuration>(jball["config"].ToString());
                this.Brightness = config.Brightness;
                this.Url = config.Url;
                this.Height = config.Height;
            }
            catch (Exception)
            {
                return false;
            }
            return true;
        }
    }

    public class WidthConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            // 在此进行你的转换逻辑
            if (value is double actualWidth)
            {
                return -actualWidth + 150;
            }
            return value; 
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NotificationEventArgs : EventArgs
    {
        public string Message { get; }

        public NotificationEventArgs(string message)
        {
            Message = message;
        }
    }

    public class MainViewModel : BindableBase
    {
        bool _USBAlive;
        public bool USBAlive { get { return _USBAlive; } set { _USBAlive = value; RaisePropertyChanged(); } }

        bool _USBScaned;
        public bool USBScaned { get { return _USBScaned; } set { _USBScaned = value; RaisePropertyChanged(); } }

        Visibility _VisibleMedia = Visibility.Collapsed;
        public Visibility VisibleMedia { get { return _VisibleMedia; } set { _VisibleMedia = value; RaisePropertyChanged(); } }

        ObservableCollection<DeviceModel> _DevicesItems = new ObservableCollection<DeviceModel>();
        public ObservableCollection<DeviceModel> DevicesItems { get { return _DevicesItems; } set { _DevicesItems = value; RaisePropertyChanged(); } }

        DeviceModel _DevicesItem;
        public DeviceModel DevicesItem { get { return _DevicesItem; } set { _DevicesItem = value; RaisePropertyChanged(); } }

        bool _IsOpenMedia;
        public bool IsOpenMedia { get { return _IsOpenMedia; } set { _IsOpenMedia = value; RaisePropertyChanged(); } }

        double _Brightness;
        public double Brightness { get { return _Brightness; } set { _Brightness = value; RaisePropertyChanged(); } }

        int _DevicesCount;
        public int DevicesCount { get { return _DevicesCount; } set { _DevicesCount = value; RaisePropertyChanged(); } }

        TextBlock _NotifyMessage;
        public TextBlock NotifyMessage { get { return _NotifyMessage; } set { _NotifyMessage = value; RaisePropertyChanged(); } }

        public string CurrentMedia { get { return player == null ? "" : player?.URL; } set { player.URL = value; ShowNotification($"当前播放: {Path.GetFileNameWithoutExtension(player.URL)}"); RaisePropertyChanged(); } }
        int _USBFPS = -1;
        public int USBFPS { get { return _USBFPS; } set { _USBFPS = value; RaisePropertyChanged(); } }
        int _CAPFPS = -1;
        public int CAPFPS { get { return _CAPFPS; } set { _CAPFPS = value; RaisePropertyChanged(); } }
        byte _RotationDir = 0;
        public byte RotationDir { get { return _RotationDir; } set { _RotationDir = value; RaisePropertyChanged(); } }

        public DelegateCommand Init { get; set; }
        public DelegateCommand<object> Connect { get; set; }
        public DelegateCommand DisConnect { get; set; }
        public DelegateCommand Rotation { get; set; }
        public DelegateCommand OpenMedia { get; set; }
        public DelegateCommand ChoiceMedia { get; set; }
        public DelegateCommand<string> ChoiceMediaUrl { get; set; }
        public DelegateCommand IsOpenChangedMedia { get; set; }
        public DelegateCommand<Flyout> ConfirmMedia { get; set; }
        public DelegateCommand<Flyout> CancelMedia { get; set; }
        public DelegateCommand<object> BrightnessCommand { get; set; }
        public DelegateCommand<object> LoadMedia { get; set; }
        public DelegateCommand<object> ClosingMedia { get; set; }

        Rectangle GetPlayerPostion()
        {
            if (player == null) return new Rectangle();
            var point = player.PointToScreen(System.Drawing.Point.Empty);
            return new Rectangle(point.X, point.Y, player.Bounds.Width, player.Bounds.Height);
        }

        AxWindowsMediaPlayer player { get; set; }
        bool CMDAvailable;
        MainWindow mainWindow { get; set; }
        int loopUSBCount = 0;
        int loopCAPCount = 0;
        NexLink nexLink = new NexLink();
        List<NexlinkDeviceInfo> nexlinkDevices = new List<NexlinkDeviceInfo>();

        nex_screen_des screendes = new nex_screen_des() { width = 240, height = 280, picw = 240, pich = 280, blocksize = 960 };
        Configuration config = new Configuration();

        void ShowNotification(string Message)
        {
            mainWindow.Dispatcher.Invoke(() =>
            {
                NotifyMessage = new TextBlock { Text = Message, SnapsToDevicePixels = true };
            });
        }

        public MainViewModel()
        {
            mainWindow = (MainWindow)Application.Current.MainWindow;
            ConfirmMedia = new DelegateCommand<Flyout>((fly) => {
                fly.IsOpen = false;
            });

            CancelMedia = new DelegateCommand<Flyout>((fly) => {
                fly.IsOpen = false;
            });

            OpenMedia = new DelegateCommand(() => {
                IsOpenMedia = true;
            });

            ChoiceMedia = new DelegateCommand(() => {
                OpenFileDialog openFileDialog = new OpenFileDialog();
                openFileDialog.Filter = "All Files (*.*)|*.*"; // 设置文件筛选器，这里是显示所有文件

                if (openFileDialog.ShowDialog() == true) // 打开文件对话框并检查用户是否点击了确定按钮
                {
                    CurrentMedia = openFileDialog.FileName; // 获取用户选择的文件路径
                }
            });

            ChoiceMediaUrl = new DelegateCommand<string>((url) => {
                CurrentMedia = url;
            });

            IsOpenChangedMedia = new DelegateCommand(() => {
                if (!IsOpenMedia)
                {
                    ThreadPool.QueueUserWorkItem((obj) =>
                    {
                        Thread.Sleep(200);
                        VisibleMedia = Visibility.Visible;
                    });
                }
                else
                {
                    VisibleMedia = Visibility.Collapsed;
                }
            });

            LoadMedia = new DelegateCommand<object>((obj) =>{
                player = obj as AxWindowsMediaPlayer;
                if (config.Load())
                {
                    Brightness = config.Brightness;
                    CurrentMedia = config.Url;
                    mainWindow.Height = 605;
                }
                else
                {
                    Brightness = 20;
                    CurrentMedia = "http://music.163.com/song/media/outer/url?id=317151";
                    mainWindow.Height = 605;
                }
                if (player != null)
                {
                    player.uiMode = "none";
                    player.stretchToFit = true;
                    //player.settings.autoStart = true;
                }
                player?.Ctlcontrols.stop();
            });

            Init = new DelegateCommand(() => {
                USBAlive = false;
                Thread.Sleep(100);
                nexlinkDevices = NexLink.ScanDevices();
                DevicesCount = nexlinkDevices.Count;
                var tempDevicesItems = new ObservableCollection<DeviceModel>();
                if (DevicesCount > 0)
                {
                    for (int i = 0; i < DevicesCount; i++)
                    {
                        tempDevicesItems.Add(new DeviceModel()
                        {
                            Name = $"{nexlinkDevices[i].Manufacturer}",
                            Index = i,
                        });
                    }
                    ShowNotification($"当前设备数量：{DevicesCount}");
                }
                else
                {
                    tempDevicesItems.Add(new DeviceModel()
                    {
                        Name = $"无设备",
                        Index = 0,
                    });
                    ShowNotification($"未检测到设备");
                }
                DevicesItems = tempDevicesItems;
                if (!DevicesItems.Contains(DevicesItem))
                    DevicesItem = DevicesItems.FirstOrDefault();
                USBScaned = true;
            });
            Init.Execute();

            Connect = new DelegateCommand<object>(async (obj) =>
            {
                var dev = obj as DeviceModel;
                USBAlive = false;
                await Task.Delay(100);
                if (DevicesCount < 1)
                {
                    ShowNotification($"当前无设备可连接");
                    return;
                }
                DevicesItem = dev;

                var ret = nexLink.Connect(nexlinkDevices.FirstOrDefault(x => x.Manufacturer == dev.Name));
                if (ret)
                    USBAlive = true;
                else
                {
                    ShowNotification($"打开设备失败");
                    return;
                }
                //NexLink.SetDirection(0);
                nexLink.GetScreenDes(ref screendes);
                if (screendes.width == 0 || screendes.height == 0 || screendes.width == 0xffff || screendes.height == 0xffff)
                {
                    USBAlive = false;
                    ShowNotification($"设备无屏幕可以显示");
                    return;
                }
                ShowNotification($"{dev.Name} 已经上线");
                string version = "";
                nexLink.GetVerDes(ref version);
                ShowNotification($"版本：{version}");
                nexLink.SetTimestamp();
                nexLink.SetBrightness(new nex_brightness_des() { brightness = Convert.ToUInt16(Brightness * 9.99), damp = 1000 });
                if (screendes.width == 0xFFFF || screendes.height == 0xFFFF)
                {
                    ShowNotification($"当前设备不需要显示");
                    return;
                }

                nexLink.SetBrightness(new nex_brightness_des() { brightness = (ushort)0, damp = 5000 });
#pragma warning disable CS4014
                Task.Run(async () =>
                {
                    while (USBAlive)
                    {
                        var recvdata = new byte[1024];
                        var count = 0;
                        nexLink.ReceiveData(ref recvdata, recvdata.Length, ref count);
                        if (count > 0)
                            ShowNotification($"{Encoding.UTF8.GetString(recvdata, 0, count).TrimEnd('\r', '\n')}");
                        await Task.Delay(100);
                    }
                });
                Task.Run(async () =>
                {
                    while (USBAlive)
                    {
                        try
                        {
                            Rectangle rect = new Rectangle();
                            mainWindow.Dispatcher.Invoke(() =>
                            {
                                rect = GetPlayerPostion();
                            });
                            nexLink.ScreenGram = CaptureScreenPart(rect);
                            loopCAPCount++;
                        }
                        catch (Exception)
                        {
                            nexLink.ScreenGram = new byte[screendes.width * screendes.height * 2];
                            await Task.Delay(100);
                        }
                    }
                });
                Task.Run(async () =>
                {
                    CMDAvailable = true;
                    while (USBAlive)
                    {
                        if (nexLink.ScreenGram != null)
                            try
                            {
                                if (CMDAvailable)
                                {
                                    mainWindow.Dispatcher.Invoke(() =>
                                    {
                                        if (RotationDir < 2)
                                        {
                                            mainWindow.Height = mainWindow.Width / screendes.width * screendes.height + 45;
                                        }
                                        else
                                        {
                                            mainWindow.Height = mainWindow.Width / screendes.height * screendes.width + 45;
                                        }
                                    });
                                    if (RotationDir < 2)
                                    {
                                        screendes.direction = RotationDir;
                                        screendes.picw = screendes.width;
                                        screendes.pich = screendes.height;
                                    }
                                    else
                                    {
                                        screendes.direction = RotationDir;
                                        screendes.picw = screendes.height;
                                        screendes.pich = screendes.width;
                                    }
                                    screendes.startx = 0; screendes.starty = 0;
                                    screendes.blocksize = 1000;
                                    nexLink.SetScreenDes(screendes);

                                    nexLink.SetBrightness(new nex_brightness_des() { brightness = Convert.ToUInt16(Brightness * 9.99), damp = 100 });
                                    CMDAvailable = false;
                                }
                                //if (nexLink.ScreenGram.Length == screendes.width * screendes.height * 2)
                                //    nexLink.TransferImageData(nexLink.ScreenGram);
                            }
                            catch (Exception e)
                            {
                                ShowNotification($"{e.Message}");
                            }
                        else
                            await Task.Delay(10);
                        loopUSBCount++;
                    }
                });
#pragma warning restore CS1998
            });

            BrightnessCommand = new DelegateCommand<object>((obj) => {
                if (obj == null)
                    return;
                CMDAvailable = true;
            });
            DisConnect = new DelegateCommand(() =>
            {
                USBAlive = false;
            });
            Rotation = new DelegateCommand(() =>
            {
                RotationDir = (byte)((RotationDir + 1) % 4);
                CMDAvailable = true;
            });

            ClosingMedia = new DelegateCommand<object>((obj) => {
                USBAlive = false;
                USBScaned = false;

                config.Brightness = Brightness;
                config.Url = CurrentMedia;
                config.Height = mainWindow.Height;
                config.Save();
            });

            ThreadPool.QueueUserWorkItem((obj) =>
            {
                Thread.Sleep(200);
                VisibleMedia = Visibility.Visible;
            });

            //Thread thread = new Thread(() => {
            //    while (true)
            //    {
            //        Thread.Sleep(1000);
            //        USBFPS = loopUSBCount;
            //        if (USBFPS > 200)
            //        {
            //            DevicesCount = 0;
            //            USBAlive = false;
            //            ShowNotification($"设备已断开");
            //        }
            //        loopUSBCount = 0;
            //        CAPFPS = loopCAPCount;
            //        loopCAPCount = 0;
            //    }
            //})
            //{ IsBackground = true };
            //thread.Start();
        }

        public byte[] ConvertTo16BitByteArray(Bitmap bitmap)
        {
            // 将图像转换为16位RGB565格式
            BitmapData bmpData = bitmap.LockBits(new Rectangle(0, 0, bitmap.Width, bitmap.Height), ImageLockMode.ReadOnly, System.Drawing.Imaging.PixelFormat.Format16bppRgb565);
            int byteCount = bmpData.Stride * bitmap.Height;
            byte[] byteArray = new byte[byteCount];
            IntPtr ptr = bmpData.Scan0;

            // 将像素数据复制到字节数组中
            System.Runtime.InteropServices.Marshal.Copy(ptr, byteArray, 0, byteCount);

            bitmap.UnlockBits(bmpData);

            return byteArray;
        }

        public byte[] CaptureScreenPart(Rectangle bounds)
        {
            // 创建一个与控件大小相同的位图
            Bitmap bitmap = new Bitmap(bounds.Width, bounds.Height);

            // 使用Graphics类绘制控件内容到位图上
            var graphics = Graphics.FromImage(bitmap);
            graphics.CopyFromScreen(new System.Drawing.Point(bounds.X, bounds.Y), System.Drawing.Point.Empty, bounds.Size);
            Bitmap scaledBitmap = new Bitmap(bitmap, new System.Drawing.Size(screendes.picw, screendes.pich));

            // 将缩小后的图像转换为16位RGB565格式的字节数组
            byte[] byteArray = ConvertTo16BitByteArray(scaledBitmap);

            // 释放资源
            bitmap.Dispose();
            // 释放资源
            scaledBitmap.Dispose();
            return byteArray;
        }
    }
}
