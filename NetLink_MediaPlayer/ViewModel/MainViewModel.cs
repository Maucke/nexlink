using AxWMPLib;
using MahApps.Metro.Controls;
using Microsoft.Win32;
using NexLinker;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
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
    public class WidthConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            // 在此进行你的转换逻辑
            if (value is double actualWidth)
            {
                return -actualWidth + 170;
            }
            return value; 
        }
        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class MainViewModel : BindableBase
    {
        double _Height = 605;
        public double Height { get { return _Height; } set { _Height = value; RaisePropertyChanged(); } }

        bool _USBAlive;
        public bool USBAlive { get { return _USBAlive; } set { _USBAlive = value; RaisePropertyChanged(); } }

        bool _USBScaned;
        public bool USBScaned { get { return _USBScaned; } set { _USBScaned = value; RaisePropertyChanged(); } }

        Visibility _VisibleMedia = Visibility.Collapsed;
        public Visibility VisibleMedia { get { return _VisibleMedia; } set { _VisibleMedia = value; RaisePropertyChanged(); } }

        bool _IsOpenMedia;
        public bool IsOpenMedia { get { return _IsOpenMedia; } set { _IsOpenMedia = value; RaisePropertyChanged(); } }

        double _Brightness = 70;
        public double Brightness { get { return _Brightness; } set { _Brightness = value; RaisePropertyChanged(); } }

        int _DevicesCount;
        public int DevicesCount { get { return _DevicesCount; } set { _DevicesCount = value; RaisePropertyChanged(); } }

        TextBlock _NotifyMessage;
        public TextBlock NotifyMessage { get { return _NotifyMessage; } set { _NotifyMessage = value; RaisePropertyChanged(); } }

        public string CurrentMedia { get { return player == null ? "" : player.URL; } set { player.URL = value; Notification($"当前播放: {Path.GetFileNameWithoutExtension(player.URL)}"); RaisePropertyChanged(); } }
        int _USBFPS;
        public int USBFPS { get { return _USBFPS; } set { _USBFPS = value; RaisePropertyChanged(); } }

        public DelegateCommand Init { get; set; }
        public DelegateCommand Connect { get; set; }
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

        void Notification(string content)
        {
            mainWindow.Dispatcher.Invoke(() =>
            {
                NotifyMessage = new TextBlock { Text = content, SnapsToDevicePixels = true };
            });
        }

        Rectangle GetPlayerPostion()
        {
            if (player == null) return new Rectangle();
            var point = player.PointToScreen(System.Drawing.Point.Empty);
            return new Rectangle(point.X, point.Y, player.Bounds.Width, player.Bounds.Height);
        }

        AxWindowsMediaPlayer player { get; set; }
        byte[] ScreenGram;
        object locker = new object();
        bool CMDAvailable;
        MainWindow mainWindow { get; set; }

        nex_screen_des screendes = new nex_screen_des() { width = 240, height = 280, blocksize = 960 };
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
                if (player != null)
                {
                    player.uiMode = "none";
                    CurrentMedia = "http://music.163.com/song/media/outer/url?id=317151";
                    player.Ctlcontrols.stop();
                    //player.settings.autoStart = true;
                }
            });

            Init = new DelegateCommand(() => {
                USBAlive = false;
                Thread.Sleep(100);
                DevicesCount = NexLink.scandevices();
                if(DevicesCount>0)
                    Notification($"当前设备数量: {DevicesCount}");
                else
                    Notification($"未检测到设备");
                USBScaned = true;
            });
            Init.Execute();

            Connect = new DelegateCommand(() =>{
                if (!USBScaned)
                    return;
                if (DevicesCount < 1)
                {
                    Notification($"当前无设备可连接");
                    return;
                }
                var ret = NexLink.initwithindex(DevicesCount - 1);
                if (ret > 0)
                    USBAlive = true;
                //NexLink.SetDirection(0);
                NexLink.GetScreenDes(ref screendes);
                NexLink.SetTimestamp();
                NexLink.SetBrightness(new nex_brightness_des() { brightness = (ushort)500, damp = 5000 });
                if (screendes.width == 0 || screendes.height == 0)
                {
                    USBAlive = false;
                    return;
                }
                else if (screendes.width == 0xFFFF || screendes.height == 0xFFFF)
                {
                    Notification($"当前设备不需要显示");
                    return;
                }

                mainWindow.Height = mainWindow.Width / screendes.width * screendes.height + 45;
                Thread threadgenerate = new Thread(() =>
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
                            lock (locker) 
                                ScreenGram = CaptureScreenPart(rect);

                        }
                        catch (Exception)
                        {
                        }
                    }
                })
                { IsBackground = true };
                threadgenerate.Start();
                Thread threadtransfer = new Thread(() =>
                {
                    while (USBAlive)
                    {
                        Stopwatch sw = new Stopwatch();
                        sw.Start();
                        if (ScreenGram != null)
                            try
                            {
                                if (CMDAvailable)
                                {
                                    NexLink.SetBrightness(new nex_brightness_des() { brightness = Convert.ToUInt16(Brightness * 9.99), damp = 100 });
                                    CMDAvailable = false;
                                }
                                var recvdata = new byte[1024];
                                var count = NexLink.receive(recvdata, 1024);
                                if (count > 0)
                                    Notification($"{Encoding.UTF8.GetString(recvdata, 0, count)}");

                                lock (locker)
                                    NexLink.TransferImageData(screendes.width, screendes.height, screendes.blocksize, ScreenGram);
                            }
                            catch (Exception e)
                            {
                                Notification($"{e.Message}");
                            }
                        else
                            Thread.Sleep(10);
                        sw.Stop();
                        USBFPS = (int)(1000 / sw.Elapsed.TotalMilliseconds);
                        if (USBFPS > 400)
                        {
                            USBAlive = false;
                            Notification($"设备已断开");
                        }
                    }
                    USBScaned = false;
                    NexLink.SetBrightness(new nex_brightness_des() { brightness = (ushort)0, damp = 5000 });
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
            });

            ThreadPool.QueueUserWorkItem((obj) =>
            {
                Thread.Sleep(200);
                VisibleMedia = Visibility.Visible;
            });
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
