using NexLink_Net;
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
        string _Description { get; set; }
        public string Description { get { return _Description; } set { _Description = value; RaisePropertyChanged(); } }
        public int Index { get; set; }

        bool _IsConnect;
        public bool IsConnect { get { return _IsConnect; } set { _IsConnect = value; RaisePropertyChanged(); } }
    }
}
