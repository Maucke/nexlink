using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using NexLink_Tool.Page;
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
                Manager.dev.Dispose();
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
                new NavigationViewItemSeparator(),
                new NavigationViewItem("Command", SymbolRegular.KeyCommand24, typeof(Command)),
                //new NavigationViewItem("Peripheral", SymbolRegular.Comment24, typeof(Peripheral)),
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
            SaveInfo saveInfo = new SaveInfo();
            JObject Jmsg = new JObject();
            TimeSpan mTimeSpan = DateTime.Now.ToUniversalTime() - new DateTime(1970, 1, 1, 0, 0, 0);
            Jmsg.Add("Timestamp", mTimeSpan.TotalSeconds);
            saveInfo.NexCommands = new List<Model.NexCommand>(Manager.commandViewModel.NexCommands);
            saveInfo.NexI2cOperators = new List<Model.NexI2cOperator>(Manager.i2cViewModel.NexI2cOperators);
            Jmsg.Add("SaveInfo", JToken.FromObject(saveInfo));

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
                try
                {
                    var saveInfo = JsonConvert.DeserializeObject<SaveInfo>(jball["SaveInfo"].ToString());
                    Manager.commandViewModel.NexCommands = new ObservableCollection<Model.NexCommand>(saveInfo.NexCommands);
                    Manager.i2cViewModel.NexI2cOperators = new ObservableCollection<Model.NexI2cOperator>(saveInfo.NexI2cOperators);
                }
                catch (Exception)
                {
                }
            }
        }
    }
}
