using NexLink_Tool.Model;
using NexLink_Tool.ViewModel;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
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
using System.Windows.Threading;

namespace NexLink_Tool.Page
{
    /// <summary>
    /// Main.xaml 的交互逻辑
    /// </summary>
    public partial class Home
    {
        public Home()
        {
            InitializeComponent();
            this.DataContext = Manager.homeViewModel;
            if (DataContext is HomeViewModel vm)
            {
                vm.Logs.CollectionChanged += Logs_CollectionChanged;
            }
            Loaded += Home_Loaded;
        }

        private void Home_Loaded(object sender, RoutedEventArgs e)
        {
            Dispatcher.BeginInvoke(new Action(() =>
            {
                var lastItem = LogListBox.Items.Cast<object>().LastOrDefault();
                if (lastItem != null)
                    LogListBox.ScrollIntoView(lastItem);

            }), DispatcherPriority.Background);
        }

        private void Logs_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
        {
            if (e.Action == NotifyCollectionChangedAction.Add)
            {
                Dispatcher.BeginInvoke(new Action(() =>
                {
                    var lastItem = LogListBox.Items.Cast<object>().LastOrDefault();
                    if (lastItem != null)
                        LogListBox.ScrollIntoView(lastItem);

                }), DispatcherPriority.Background);
            }
        }
    }
}
