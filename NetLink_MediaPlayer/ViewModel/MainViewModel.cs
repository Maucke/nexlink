using AxWMPLib;
using MahApps.Metro.Controls;
using Microsoft.Win32;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NexLinker;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Threading;

namespace NetLink_MediaPlayer.ViewModel
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

    public class NotificationManager
    {
        private Queue<string> notificationsQueue;
        private bool showingNotification;
        private Timer timer;

        public event EventHandler<NotificationEventArgs> NotificationDisplayed;
        public event EventHandler IdleTimerElapsed;

        public NotificationManager()
        {
            notificationsQueue = new Queue<string>();
            showingNotification = false;
        }

        public void ShowNotification(string message)
        {
            notificationsQueue.Enqueue(message);
            if (!showingNotification)
            {
                ShowNextNotification();
            }

            // Reset the timer for idle detection
            ResetIdleTimer();
        }

        private void ShowNextNotification()
        {
            if (notificationsQueue.Count > 0)
            {
                showingNotification = true;
                string message = notificationsQueue.Dequeue();
                Console.WriteLine($"Showing notification: {message}");

                // Trigger the event to notify external subscribers
                OnNotificationDisplayed(message);

                // Set timer based on message length
                int timerInterval = message.Length > 10 ? 2500 : 800;
                if (timer == null)
                    timer = new Timer(OnTimerElapsed, null, timerInterval, Timeout.Infinite);
            }
            else
            {
                showingNotification = false;
                // Start the idle timer if queue is empty
                StartIdleTimer();
            }
        }

        private void OnTimerElapsed(object state)
        {
            Console.WriteLine("Notification closed.");
            timer.Dispose(); // Dispose the timer to release resources

            // Show the next notification if any
            ShowNextNotification();
        }

        private Timer idleTimer;

        private void StartIdleTimer()
        {
            if (idleTimer == null)
                // Start a timer to check for idle period
                idleTimer = new Timer(OnIdleTimerElapsedInternal, null, 5000, Timeout.Infinite);
        }

        private void ResetIdleTimer()
        {
            // Reset the idle timer
            idleTimer?.Change(5000, Timeout.Infinite);
        }

        private void OnIdleTimerElapsedInternal(object state)
        {
            Console.WriteLine("Automatic notification after idle period.");

            // Trigger external event
            OnIdleTimerElapsed();
        }

        protected virtual void OnNotificationDisplayed(string message)
        {
            NotificationDisplayed?.Invoke(this, new NotificationEventArgs(message));
        }

        protected virtual void OnIdleTimerElapsed()
        {
            IdleTimerElapsed?.Invoke(this, EventArgs.Empty);
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

        public string CurrentMedia { get { return player == null ? "" : player?.URL; } set { player.URL = value; manager.ShowNotification($"当前播放: {Path.GetFileNameWithoutExtension(player.URL)}"); RaisePropertyChanged(); } }
        int _USBFPS = -1;
        public int USBFPS { get { return _USBFPS; } set { _USBFPS = value; RaisePropertyChanged(); } }
        int _CAPFPS = -1;
        public int CAPFPS { get { return _CAPFPS; } set { _CAPFPS = value; RaisePropertyChanged(); } }

        public DelegateCommand Init { get; set; }
        public DelegateCommand<object> Connect { get; set; }
        public DelegateCommand DisConnect { get; set; }
        public DelegateCommand OpenMedia { get; set; }
        public DelegateCommand ChoiceMedia { get; set; }
        public DelegateCommand<string> ChoiceMediaUrl { get; set; }
        public DelegateCommand IsOpenChangedMedia { get; set; }
        public DelegateCommand<Flyout> ConfirmMedia { get; set; }
        public DelegateCommand<Flyout> CancelMedia { get; set; }
        public DelegateCommand<object> BrightnessCommand { get; set; }
        public DelegateCommand<object> LoadMedia { get; set; }
        public DelegateCommand<object> ClosingMedia { get; set; }

        NotificationManager manager = new NotificationManager();

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

        nex_screen_des screendes = new nex_screen_des() { width = 240, height = 280, blocksize = 960 };
        Thread threadreceive = null, threadgenerate = null, threadtransfer = null;
        Configuration config = new Configuration();

        public MainViewModel()
        {
            NexLink.Init();

            manager.NotificationDisplayed+= (sender, args) =>
            {
                mainWindow.Dispatcher.Invoke(() =>
                {
                    NotifyMessage = new TextBlock { Text = args.Message, SnapsToDevicePixels = true };
                });
            };

            manager.IdleTimerElapsed += (sender, args) =>
            {
                string notification = "";
                if (USBAlive)
                    notification = ($"{DevicesItems[nexLink.GetIndex()].Name} 在线");
                else
                    notification = ($"无设备在线");
                mainWindow.Dispatcher.Invoke(() =>
                {
                    NotifyMessage = new TextBlock { Text = notification, SnapsToDevicePixels = true };
                });
            };

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
                    mainWindow.Height = config.Height;
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
                    //player.settings.autoStart = true;
                }
                player?.Ctlcontrols.stop();
            });

            Init = new DelegateCommand(() => {
                USBAlive = false;
                Thread.Sleep(100);
                try
                {
                    threadreceive?.Abort();
                    threadgenerate?.Abort();
                    threadtransfer?.Abort();
                }
                catch (Exception e)
                {
                    manager.ShowNotification($"{e.Message}");
                }
                var deviceInfo = NexLink.ScanDevices();
                DevicesCount = deviceInfo.Count;
                var tempDevicesItems = new ObservableCollection<DeviceModel>();
                if (DevicesCount > 0)
                {
                    for (int i = 0; i < DevicesCount; i++)
                    {
                        tempDevicesItems.Add(new DeviceModel()
                        {
                            Name = $"{deviceInfo[i].manufacturer}",
                            Index = i,
                        });
                    }
                    manager.ShowNotification($"当前设备数量：{DevicesCount}");
                }
                else
                {
                    tempDevicesItems.Add(new DeviceModel()
                    {
                        Name = $"无设备",
                        Index = 0,
                    });
                    manager.ShowNotification($"未检测到设备");
                }
                DevicesItems = tempDevicesItems;
                if (!DevicesItems.Contains(DevicesItem))
                    DevicesItem = DevicesItems.FirstOrDefault();
                USBScaned = true;
            });
            Init.Execute();

            Connect = new DelegateCommand<object>((obj) =>
            {
                var dev = obj as DeviceModel;
                if (!USBScaned || USBAlive)
                {
                    Init.Execute();
                }
                if (DevicesCount < 1)
                {
                    manager.ShowNotification($"当前无设备可连接");
                    return;
                }
                DevicesItem = dev;
                var ret = nexLink.OpenDevice(dev.Index);
                if (ret)
                    USBAlive = true;
                else
                {
                    manager.ShowNotification($"打开设备失败");
                    return;
                }
                //NexLink.SetDirection(0);
                nexLink.GetScreenDes(ref screendes);
                nexLink.SetScreenDes(screendes);
                if (screendes.width == 0 || screendes.height == 0)
                {
                    USBAlive = false;
                    return;
                }
                manager.ShowNotification($"{dev.Name} 已经上线");
                string version = "";
                nexLink.GetVerDes(ref version);
                manager.ShowNotification($"版本：{version}");
                nexLink.SetTimestamp();
                nexLink.SetBrightness(new nex_brightness_des() { brightness = Convert.ToUInt16(Brightness * 9.99), damp = 1000 });
                threadreceive = new Thread(() =>
                {
                    while (USBAlive)
                    {
                        var recvdata = new byte[1024];
                        var count = 0;
                        nexLink.ReceiveData(ref recvdata, recvdata.Length, ref count);
                        if (count > 0)
                            manager.ShowNotification($"{Encoding.UTF8.GetString(recvdata, 0, count).TrimEnd('\r', '\n')}");
                        Thread.Sleep(100);
                    }
                    threadreceive = null;
                })
                { IsBackground = true };
                threadreceive.Start();
                if (screendes.width == 0xFFFF || screendes.height == 0xFFFF)
                {
                    manager.ShowNotification($"当前设备不需要显示");
                    return;
                }

                mainWindow.Height = mainWindow.Width / screendes.width * screendes.height + 45;
                threadgenerate = new Thread(() =>
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
                            Thread.Sleep(100);
                        }
                    }
                    threadgenerate = null;
                })
                { IsBackground = true };
                threadgenerate.Start();
                threadtransfer = new Thread(() =>
                {
                    while (USBAlive)
                    {
                        if (nexLink.ScreenGram != null)
                            try
                            {
                                if (CMDAvailable)
                                {
                                    nexLink.SetBrightness(new nex_brightness_des() { brightness = Convert.ToUInt16(Brightness * 9.99), damp = 100 });
                                    CMDAvailable = false;
                                }
                                if (nexLink.ScreenGram.Length == screendes.width * screendes.height * 2)
                                    nexLink.TransferImageData(screendes.width, screendes.height, screendes.blocksize, nexLink.ScreenGram);
                            }
                            catch (Exception e)
                            {
                                manager.ShowNotification($"{e.Message}");
                            }
                        else
                            Thread.Sleep(10);
                        loopUSBCount++;
                    }
                    USBScaned = false;
                    nexLink.SetBrightness(new nex_brightness_des() { brightness = (ushort)0, damp = 5000 });
                    threadtransfer = null;
                })
                { IsBackground = true };
                threadtransfer.Start();
            });

            BrightnessCommand = new DelegateCommand<object>((obj) => {
                if (obj == null)
                    return;
                CMDAvailable = true;
            });

            ClosingMedia = new DelegateCommand<object>((obj) => {
                USBAlive = false;

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

            Thread thread = new Thread(() => {
                while (true)
                {
                    Thread.Sleep(1000);
                    USBFPS = loopUSBCount;
                    if (USBFPS > 200)
                    {
                        DevicesCount = 0;
                        USBAlive = false;
                        manager.ShowNotification($"设备已断开");
                    }
                    loopUSBCount = 0;
                    CAPFPS = loopCAPCount;
                    loopCAPCount = 0;
                }
            })
            { IsBackground = true };
            thread.Start();
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
            Bitmap scaledBitmap = new Bitmap(bitmap, new System.Drawing.Size(screendes.width, screendes.height));

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
