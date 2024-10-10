using Hexconverters;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NexLink_Tool.Model
{
    internal class NexCommand : BindableBase
    {
        byte _Addr;
        public byte Addr { get { return _Addr; } set { _Addr = value; RaisePropertyChanged(); } }

        int _Size;
        public int Size { get { return _Size; } set { _Size = value; RaisePropertyChanged(); } }

        string _Data;
        public string Data { get { return _Data; } set { _Data = value; RaisePropertyChanged(); } }

        string _AsciiData;
        public string AsciiData { get { return _AsciiData; } set { _AsciiData = value; RaisePropertyChanged(); } }
    }
}
