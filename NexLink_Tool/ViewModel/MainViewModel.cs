using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NexLink_Tool.Page;
using NexLinker;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Xml.Linq;
using Wpf.Ui.Controls;
using File = System.IO.File;

namespace NexLink_Tool.ViewModel
{
    internal class MainViewModel : BindableBase
    {
        internal NavigationView navigation;
        internal MainViewModel()
        {
            Manager.mainViewModel = this;
            LoadProject();
            Loaded = new DelegateCommand<object>((o) => {
                if (o is NavigationView navigation)
                {
                    this.navigation = navigation;
                    navigation.Navigate(typeof(Home));
                }
            });

            Closed = new DelegateCommand<object>((o) => {
                SaveProject();
                if(Manager.nexLink.IsConnected)
                    Manager.nexLink.CloseDevice();
            });
            Task.Run(async () =>
            {
                while(true)
                {
                    await Task.Delay(1000);
                }
            });
        }
        public DelegateCommand<object> Loaded { get; set; }
        public DelegateCommand<object> Closed { get; set; }

        private ObservableCollection<object> _menuItems = new ObservableCollection<object>()
            {
                new NavigationViewItem("Home", SymbolRegular.Home24, typeof(Home)),
                new NavigationViewItem("Command", SymbolRegular.KeyCommand24, typeof(Command)),
                new NavigationViewItemSeparator(),
            };
        public ObservableCollection<object> MenuItems
            { get { return _menuItems; } set { _menuItems = value; RaisePropertyChanged(); } }

        private ObservableCollection<object> _footermenuItems = new ObservableCollection<object>()
            {
                new NavigationViewItem("Settings", SymbolRegular.LauncherSettings24, typeof(Setting))
            };
        public ObservableCollection<object> FooterMenuItems
        { get { return _footermenuItems; } set { _footermenuItems = value; RaisePropertyChanged(); } }

        public void SaveProject()
        {
            JObject Jmsg = new JObject();
            TimeSpan mTimeSpan = DateTime.Now.ToUniversalTime() - new DateTime(1970, 1, 1, 0, 0, 0);
            Jmsg.Add("Timestamp", mTimeSpan.TotalSeconds);

            //Jmsg.Add("SaveInfo", JToken.FromObject(Manager.si));
            string filePath = AppDomain.CurrentDomain.BaseDirectory + "config.proj";
            File.WriteAllText(filePath, Jmsg.ToString());
        }

        void LoadProject()
        {
            string filePath = AppDomain.CurrentDomain.BaseDirectory + "config.proj";
            if (File.Exists(filePath))
            {
                string json = File.ReadAllText(filePath, Encoding.UTF8);
                JObject jball = (JObject)JsonConvert.DeserializeObject(json);
            }
            else
            {
            }
        }
    }
}
