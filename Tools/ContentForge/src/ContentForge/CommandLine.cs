using System;
using System.Diagnostics;

namespace ContentForge
{
	public class CommandLine
	{
		public static string Run(string FileName, string Args, bool ShowWnd)
		{
			ProcessStartInfo Info = new ProcessStartInfo(FileName);
			Info.UseShellExecute = false;
			Info.Arguments = Args;
			Info.CreateNoWindow = !ShowWnd;
			Info.RedirectStandardInput = true;
			Info.RedirectStandardOutput = true;

			using (Process Process = Process.Start(Info))
			{
				return Process.StandardOutput.ReadToEnd();
			}
		}
	}
}
