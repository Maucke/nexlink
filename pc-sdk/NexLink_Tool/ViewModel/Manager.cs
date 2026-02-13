using NexLink;
using NexLink_Tool.Model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media.Animation;
using Wpf.Ui;
using Wpf.Ui.Controls;
using Wpf.Ui.Extensions;

namespace NexLink_Tool.ViewModel
{
    internal static class Manager
    { 
        internal static MainViewModel mainViewModel;
        //internal static Logger logger = new Logger("log.txt");
        internal static SnackbarService snackbarService = new SnackbarService();
        internal static ContentDialogService contentDialogService = new ContentDialogService();

        internal static NexLinkDevice dev;
        internal static HomeViewModel homeViewModel = new HomeViewModel();
        internal static PeripheralViewModel i2cViewModel = new PeripheralViewModel();
        internal static CommandViewModel commandViewModel = new CommandViewModel();
        internal static SettingViewModel settingViewModel = new SettingViewModel();

        internal static void BeginInvokeAction(Action action)
        {
            if (Application.Current == null) return;

            if (Application.Current.Dispatcher.CheckAccess())
                action();
            else
                Application.Current.Dispatcher.BeginInvoke(action);
        }
        internal static async Task<ContentDialogResult> ShowDialog(object content, string title)
        {
#if true
            ContentDialogResult result = await contentDialogService.ShowSimpleDialogAsync(
                new SimpleContentDialogCreateOptions()
                {
                    Title = title,
                    Content = content,
                    PrimaryButtonText = "OK",
                    CloseButtonText = "Cancel",
                }
            );
#else
            ContentDialogResult result = ContentDialogResult.None;
            //await Task.Run(() => {
                
            //});
#endif
            return result;
        }
        public static void AppendLog(string msg, LogLevel level)
        {
            var Logs = Manager.homeViewModel.Logs;
            if (Application.Current.Dispatcher.CheckAccess())
            {
                Logs.Add(new LogItem
                {
                    Time = DateTime.Now.ToString("HH:mm:ss.fff"),
                    Message = msg,
                    Level = level
                });
            }
            else
            {
                Application.Current.Dispatcher.Invoke(() =>
                {
                    Logs.Add(new LogItem
                    {
                        Time = DateTime.Now.ToString("HH:mm:ss.fff"),
                        Message = msg,
                        Level = level
                    });
                });
            }
        }
        
        internal static void ShowNoti(string content, ControlAppearance type = ControlAppearance.Info, int timeouts = 2)
        {
            try
            {
                switch (type)
                {
                    case ControlAppearance.Danger:
                        AppendLog("[UI] " + content, LogLevel.Error);
                        break;
                    case ControlAppearance.Caution:
                        AppendLog("[UI] " + content, LogLevel.Warn);
                        break;
                    default:
                        AppendLog("[UI] " + content, LogLevel.Info);
                        break;
                }
                BeginInvokeAction(() =>
                snackbarService.Show(
                           type.ToString(),
                           content,
                           type,
                           new SymbolIcon(SymbolRegular.Bot24),
                           TimeSpan.FromSeconds(timeouts)
                       )
                    );
            }
            catch (Exception)
            {
            }
        }
        internal static void ShowNoti(string content, string title, ControlAppearance type = ControlAppearance.Info, int timeouts = 2)
        {
            try
            {
                switch (type)
                {
                    case ControlAppearance.Danger:
                        AppendLog("[UI] " + title + content, LogLevel.Error);
                        break;
                    case ControlAppearance.Caution:
                        AppendLog("[UI] " + title + content, LogLevel.Warn);
                        break;
                    default:
                        AppendLog("[UI] " + title + content, LogLevel.Info);
                        break;
                }
                BeginInvokeAction(() =>
                snackbarService.Show(
                           title,
                           content,
                           type,
                           new SymbolIcon(SymbolRegular.Bot24),
                           TimeSpan.FromSeconds(timeouts)
                       )
                    );
            }
            catch (Exception)
            {
            }
        }
    }
}
