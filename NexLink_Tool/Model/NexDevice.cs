using NexLinker;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NexLink_Tool.Model
{
    internal class NexDevice : BindableBase
    {
        public string Name { get; set; }
        public int Index { get; set; }

        bool _IsConnect;
        public bool IsConnect { get { return _IsConnect; } set { _IsConnect = value; RaisePropertyChanged(); } }
    }
}
