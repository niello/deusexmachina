using System.Diagnostics;

namespace CreatorIDE
{
    public class CommandLine
    {
        public static string Run(string fileName, string args, bool showWnd)
        {
			var info = new ProcessStartInfo(fileName)
			               {
			                   UseShellExecute = false,
			                   Arguments = args,
			                   CreateNoWindow = !showWnd,
			                   RedirectStandardInput = true,
			                   RedirectStandardOutput = true
			               };

            using (Process process = Process.Start(info))
			{
				return process.StandardOutput.ReadToEnd();
			}
		}
    }
}
