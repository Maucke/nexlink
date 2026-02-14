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
    public List<NexI2cOperator> NexI2cOperators { get; set; }

}
