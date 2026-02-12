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

namespace NexLink_Tool.ViewModel
{
    internal class HomeViewModel : BindableBase
    {
        internal HomeViewModel()
        {
            Unloaded = new DelegateCommand<object>((o) => { });
        }
        public ObservableCollection<LogItem> Logs { get; } = new();

        public DelegateCommand<object> Unloaded { get; set; }
    }
}
