using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NexLink_Tool.Model
{
    internal class NexI2cOperator : BindableBase
    {
        byte _SlaveAddr { get; set; }
        public byte SlaveAddr { get { return _SlaveAddr; } set { _SlaveAddr = value; RaisePropertyChanged(); } }

        byte _RegAddr { get; set; }
        public byte RegAddr { get { return _RegAddr; } set { _RegAddr = value; RaisePropertyChanged(); } }

        byte _Size { get; set; }
        public byte Size { get { return _Size; } set { _Size = value; RaisePropertyChanged(); } }

        string _Data { get; set; }
        public string Data { get { return _Data; } set { _Data = value; RaisePropertyChanged(); } }

    }
}
