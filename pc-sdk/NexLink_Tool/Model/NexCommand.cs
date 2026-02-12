using Hexconverters;
using Newtonsoft.Json;
using NexLink;
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
        NexLinkCmd _Cmd;
        public NexLinkCmd Cmd { get { return _Cmd; } set { _Cmd = value; RaisePropertyChanged(); } }

        string _Data;
        public string Data { get { return _Data; } set { _Data = value; RaisePropertyChanged(); } }

        string _AsciiData;
        public string AsciiData { get { return _AsciiData; } set { _AsciiData = value; RaisePropertyChanged(); } }
    }
}
