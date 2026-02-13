using Hexconverters;
using NexLink_Tool.Model;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Media;

namespace NexLink_Tool.ViewModel
{
    internal class HomeViewModel : BindableBase
    {
        internal HomeViewModel()
        {
            Unloaded = new DelegateCommand<object>((o) => { });
        }
        public ObservableCollection<LogItem> Logs { get; } = new();

        private ImageSource _displayImage;
        public ImageSource DisplayImage
        {
            get => _displayImage;
            set
            {
                _displayImage = value;
                RaisePropertyChanged();
            }
        }
        public DelegateCommand<object> Unloaded { get; set; }
    }
}
