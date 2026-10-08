using NexLink_Tool.ViewModel;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Wpf.Ui.Controls;

namespace NexLink_Tool
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : FluentWindow
    {
        private bool _allowClose;

        public MainWindow()
        {
            InitializeComponent();
            Manager.snackbarService.SetSnackbarPresenter(SnackbarPresenter);
            Manager.contentDialogService.SetDialogHost(RootContentDialog);
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            if (_allowClose)
            {
                base.OnClosing(e);
                return;
            }

            /* 关窗前要往设备发一张全黑图（否则设备停在最后一帧画面上），
               而发一帧要几百毫秒 —— 不能在 UI 线程等。所以先拦下这次关闭，
               把收尾丢到后台，收完再真正关闭。
               SaveProject 等仍由下面 base.OnClosing(e) 触发的 Closing 命令执行。 */
            e.Cancel = true;
            Hide();

            base.OnClosing(e);

            Task.Run(() =>
            {
                try
                {
                    Manager.homeViewModel.StopAllStreamsAndWait(3000); /* 停流 + 等收尾 */
                    Manager.dev?.ClearScreen();                        /* 后台发黑屏 */
                }
                catch
                {
                }

                Dispatcher.BeginInvoke(new Action(() =>
                {
                    _allowClose = true;
                    Close();
                    Application.Current.Shutdown();
                }));
            });
        }
    }
}
