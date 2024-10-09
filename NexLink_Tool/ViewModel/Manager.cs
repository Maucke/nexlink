using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Wpf.Ui.Controls;
using Wpf.Ui;
using Wpf.Ui.Extensions;
using System.Windows.Controls;
using System.Windows.Media.Animation;
using System.Collections.ObjectModel;
using System.Windows;
using NexLinker;

namespace NexLink_Tool.ViewModel
{
    internal static class Manager
    { 
        internal static MainViewModel mainViewModel;
        //internal static Logger logger = new Logger("log.txt");
        internal static SnackbarService snackbarService = new SnackbarService();
        internal static ContentDialogService contentDialogService = new ContentDialogService();

        internal static HomeViewModel homeViewModel = new HomeViewModel();
        internal static SettingViewModel settingViewModel = new SettingViewModel();
        internal static CommandViewModel commandViewModel = new CommandViewModel();
        internal static NexLink nexLink = new NexLink();

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
#if false
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
#endif
            return result;
        }
        internal static void ShowNoti(string content, ControlAppearance type = ControlAppearance.Info, int timeouts = 2)
        {
            try
            {
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
