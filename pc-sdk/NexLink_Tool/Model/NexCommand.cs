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
        internal NexCommand()
        { }
        internal NexCommand(string name, NexLinkCmd cmd, string data = "")
        {
            Name = name;
            Cmd = cmd;
            Data = data;
        }

        string _Name;
        public string Name { get { return _Name; } set { _Name = value; RaisePropertyChanged(); } }

        NexLinkCmd _Cmd;
        public NexLinkCmd Cmd { get { return _Cmd; } set { _Cmd = value; RaisePropertyChanged(); } }

        string _Data;
        public string Data { get { return _Data; } set { _Data = value; RaisePropertyChanged(); } }

        string _AsciiData;

        public string AsciiData { get { return _AsciiData; } set { _AsciiData = value; RaisePropertyChanged(); } }
    }
}
