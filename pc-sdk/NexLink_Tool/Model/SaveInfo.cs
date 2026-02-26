using NexLink_Tool.Model;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

internal class SaveInfo
{ 
    public string HomeVal { get; set; }
    public List<NexCommand> NexCommands { get; set; }
    public List<NexCommand> CustomNexCommands { get; set; }

    public bool ThemeDark { get; set; }
    public string LastDevice { get; set; }

    public string UartChn { get; set; }
    public uint UartBaudRate { get; set; }
    public string UartData { get; set; }

    public string I2cChn { get; set; }
    public uint I2cClock { get; set; }
    public List<NexI2cOperator> NexI2cOperators { get; set; }

    public string SpiChn { get; set; }
    public uint SpiClock { get; set; }
    public int SpiMode { get; set; }
    public string SpiData { get; set; }


}
