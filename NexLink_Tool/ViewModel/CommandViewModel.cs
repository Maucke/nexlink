using Hexconverters;
using Microsoft.Win32;
using NexLink_Tool.Model;
using NexLink_Net;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using NexLink_NET;

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
                            var ret = NexLink.control_in((byte)cmd.Addr, rawdata, (ushort)rawdata.Length);
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
                            //var ret = Manager.nexLink.ControlSetData((NEX_BREQ)cmd.Addr, Hexstring.GetBytes(cmd.Data));
                            //if (ret < 0)
                            //    Manager.ShowNoti("Set data failed", Wpf.Ui.Controls.ControlAppearance.Caution);
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
            ShowPic = new DelegateCommand<object>((o) => {
                byte rotation = Convert.ToByte(o);
                if (Manager.nexLink.Screendes.width == 0 || Manager.nexLink.Screendes.width == 0xffff || Manager.nexLink.Screendes.height == 0 || Manager.nexLink.Screendes.height == 0xffff)
                {
                    Manager.ShowNoti("Device have not screen");return;
                }
                // 创建一个 OpenFileDialog 实例
                OpenFileDialog openFileDialog = new OpenFileDialog();
                {
                    // 设置过滤器以仅选择图片文件
                    openFileDialog.Filter = "Image Files|*.jpg;*.jpeg;*.png;*.bmp;*.gif";

                    // 显示对话框并检查用户是否选择了文件
                    if (openFileDialog.ShowDialog().Value == true)
                    {
                        try
                        {
                            Task.Run(() =>
                            {
                                nex_screen_des screendes = new nex_screen_des();
                                nex_picture_des picturedes = new nex_picture_des();
                                Manager.nexLink.GetScreenDes(ref screendes);
                                //screendes.width = 140; screendes.height = 120;
                                //screendes.startx = 70; screendes.starty = 60; screendes.direction = 2;
                                if (rotation < 2)
                                {
                                    picturedes.direction = rotation;
                                    picturedes.startx = 0; picturedes.starty = 0;
                                    picturedes.picw = screendes.width;
                                    picturedes.pich = screendes.height;
                                }
                                else
                                {
                                    picturedes.direction = rotation;
                                    picturedes.startx = 0; picturedes.starty = 0;
                                    picturedes.picw = screendes.height;
                                    picturedes.pich = screendes.width;
                                }
                                // 加载选中的图片文件并转换为 Bitmap
                                Bitmap bitmap = new Bitmap(openFileDialog.FileName);
                                Bitmap scaledBitmap = CropAndMaintainAspectRatio(bitmap, picturedes.picw, picturedes.pich);
                          
                                Manager.nexLink.TransferImageData(picturedes, ConvertTo16BitByteArray(scaledBitmap));
                            });
                        }
                        catch (Exception e)
                        {
                            Manager.ShowNoti(e.Message);
                        }
                    }
                }
            });
            Brightness = new DelegateCommand<object>((o) => {
                if (Manager.nexLink.Screendes.width == 0 || Manager.nexLink.Screendes.width == 0xffff || Manager.nexLink.Screendes.height == 0 || Manager.nexLink.Screendes.height == 0xffff)
                {
                    Manager.ShowNoti("Device have not screen"); return;
                }
                nex_brightness_des brightnessdes = new nex_brightness_des();
                Manager.nexLink.GetBrightness(ref brightnessdes);
                brightnessdes.brightness = (ushort)((brightnessdes.brightness + 50) % 999);
                if (brightnessdes.brightness < 5)
                    brightnessdes.brightness = 999;
                Manager.ShowNoti($"Current brightness {(int)(brightnessdes.brightness / 9.99f)}%");
                brightnessdes.damp = 100;
                Manager.nexLink.SetBrightness(brightnessdes);
            });
            Test = new DelegateCommand<object>((o) => {
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
                                await Task.Delay(10);
                            }
                        }
                    }

                    await Task.Delay(100);
                }
            });
        }
        public Bitmap CropAndMaintainAspectRatio(Bitmap originalBitmap, int targetWidth, int targetHeight)
        {
            // 计算原始图片的宽高比
            float originalAspect = (float)originalBitmap.Width / originalBitmap.Height;
            float targetAspect = (float)targetWidth / targetHeight;

            int newWidth, newHeight;

            if (originalAspect > targetAspect)
            {
                // 原始图像更宽，按高度缩放
                newHeight = targetHeight;
                newWidth = (int)(newHeight * originalAspect);
            }
            else
            {
                // 原始图像更高或等于目标宽高比，按宽度缩放
                newWidth = targetWidth;
                newHeight = (int)(newWidth / originalAspect);
            }

            // 创建缩放后的 Bitmap
            Bitmap scaledBitmap = new Bitmap(originalBitmap, new Size(newWidth, newHeight));

            // 计算裁剪区域
            int cropX = (scaledBitmap.Width - targetWidth) / 2;
            int cropY = (scaledBitmap.Height - targetHeight) / 2;

            // 裁剪图像
            Rectangle cropArea = new Rectangle(cropX, cropY, targetWidth, targetHeight);
            Bitmap croppedBitmap = new Bitmap(targetWidth, targetHeight);

            using (Graphics g = Graphics.FromImage(croppedBitmap))
            {
                g.DrawImage(scaledBitmap, new Rectangle(0, 0, targetWidth, targetHeight), cropArea, GraphicsUnit.Pixel);
            }

            // 释放资源
            scaledBitmap.Dispose();

            return croppedBitmap;
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
        public DelegateCommand<object> ShowPic { get; set; }
        public DelegateCommand<object> Brightness { get; set; }
        public DelegateCommand<object> Test { get; set; }

        public DelegateCommand<object> SyncTime { get; set; }
        public DelegateCommand<object> GetName { get; set; }
        public DelegateCommand<object> GetLog { get; set; }
    }
}
