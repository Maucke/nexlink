using NexLink_Tool.ViewModel;
using System;
using System.Collections.Generic;
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

namespace NexLink_Tool.Page
{
    /// <summary>
    /// I2c.xaml 的交互逻辑
    /// </summary>
    public partial class Peripheral
    {
        public Peripheral()
        {
            InitializeComponent();
            this.DataContext = Manager.peripheralViewModel;
        }
        private void tbx_uartlog_TextChanged(object sender, TextChangedEventArgs e)
        {
            tbx_uartlog.ScrollToEnd();
        }
    }
}
