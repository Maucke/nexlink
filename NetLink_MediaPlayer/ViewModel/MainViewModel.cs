using AxWMPLib;
using MahApps.Metro.Controls;
using Microsoft.Win32;
using NexLinker;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
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
    public class MainViewModel : BindableBase
    {
        bool _USBAlive;
        public bool USBAlive { get { return _USBAlive; } set { _USBAlive = value; RaisePropertyChanged(); } }

        bool _USBScaned;
        public bool USBScaned { get { return _USBScaned; } set { _USBScaned = value; RaisePropertyChanged(); } }

        Visibility _VisibleMedia;
        public Visibility VisibleMedia { get { return _VisibleMedia; } set { _VisibleMedia = value; RaisePropertyChanged(); } }

        bool _IsOpenMedia;
        public bool IsOpenMedia { get { return _IsOpenMedia; } set { _IsOpenMedia = value; RaisePropertyChanged(); } }

        double _Brightness = 70;
        public double Brightness { get { return _Brightness; } set { _Brightness = value; RaisePropertyChanged(); } }

        int _DevicesCount;
        public int DevicesCount { get { return _DevicesCount; } set { _DevicesCount = value; RaisePropertyChanged(); } }

        TextBlock _NotifyMessage;
        public TextBlock NotifyMessage { get { return _NotifyMessage; } set { _NotifyMessage = value; RaisePropertyChanged(); } }

        public string CurrentMedia { get { return player == null ? "" : player.URL; } set { player.URL = value; RaisePropertyChanged(); } }

        public DelegateCommand Init { get; set; }
        public DelegateCommand Connect { get; set; }
        public DelegateCommand DisConnect { get; set; }
        public DelegateCommand OpenMedia { get; set; }
        public DelegateCommand ChoiceMedia { get; set; }
        public DelegateCommand IsOpenChangedMedia { get; set; }
        public DelegateCommand<Flyout> ConfirmMedia { get; set; }
        public DelegateCommand<Flyout> CancelMedia { get; set; }
        public DelegateCommand<object> BrightnessCommand { get; set; }
        public DelegateCommand<object> LoadMedia { get; set; }

        void Notification(string content)
        {
            NotifyMessage = new TextBlock { Text = content, SnapsToDevicePixels = true };
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
                                                                       // 在这里可以使用selectedFilePath进行后续操作，比如显示文件路径或者读取文件内容
                }
            });

            IsOpenChangedMedia = new DelegateCommand(() => {
                if (!IsOpenMedia)
                {
                    ThreadPool.QueueUserWorkItem((obj) =>
                    {
                        var aplayer = obj as AxWindowsMediaPlayer;
                        Thread.Sleep(200);
                        VisibleMedia = Visibility.Visible;
                    }, player);
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
                NexLink.close();
                DevicesCount = NexLink.scandevices();
                Notification($"当前设备数量: {DevicesCount}");
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
                NexLink.SetDirection(0);
                NexLink.SetTimestamp();
                NexLink.SetBrightness(new nex_brightness_des() { brightness = (ushort)500, damp = 5000 });
                Thread threadgenerate = new Thread(() =>
                {
                    while (USBAlive)
                    {
                        Rectangle rect = new Rectangle();

                        mainWindow.Dispatcher.Invoke(() =>
                        {
                            rect = GetPlayerPostion();
                        });
                        lock (locker) 
                            ScreenGram = CaptureScreenPart(rect);
                    }
                })
                { IsBackground = true };
                threadgenerate.Start();
                Thread threadtransfer = new Thread(() =>
                {
                    while (USBAlive)
                    {
                        if (ScreenGram != null)
                            try
                            {
                                if (CMDAvailable)
                                {
                                    NexLink.SetBrightness(new nex_brightness_des() { brightness = Convert.ToUInt16(Brightness * 9.99), damp = 500 });
                                    CMDAvailable = false;
                                }

                                lock (locker)
                                    NexLink.TransferImageData(ScreenGram);
                            }
                            catch (Exception)
                            {
                                USBAlive = false;
                                USBScaned = false;
                                Thread.Sleep(10);
                            }
                        else
                            Thread.Sleep(10);
                    }
                    USBScaned = false;
                    NexLink.close();
                })
                { IsBackground = true };
                threadtransfer.Start();
            });
            BrightnessCommand = new DelegateCommand<object>((obj) => {
                if (obj == null)
                    return;
                CMDAvailable = true;
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
            Bitmap scaledBitmap = new Bitmap(bitmap, new System.Drawing.Size(NexLink.SCR_WIDTH, NexLink.SCR_HEIGHT));

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
