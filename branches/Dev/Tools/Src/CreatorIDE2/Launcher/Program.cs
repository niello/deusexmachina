using System;
using System.Diagnostics;
using System.IO;
using CreatorIDE.Package;
using Microsoft.VisualStudio.Shell;
using Microsoft.Win32;

namespace CreatorIDE.Launcher
{
    internal static class Program
    {
        private const string ConfigRoot =
#if RELEASE
            @"SOFTWARE\Microsoft\VisualStudio\9.0";
#else
            @"Software\Microsoft\VisualStudio\9.0Exp\Configuration";
#endif

        private static void Main(string[] args)
        {
            Exception exception = null;
            bool wait = false;
            if (args != null && args.Length > 0)
            {
                var arg = args[0];
                if (arg != null)
                    arg = arg.TrimStart('-', '/').Trim().ToLowerInvariant();
                switch (arg)
                {
                    case "":
                    case null:
                        try
                        {
                            RegisterAndRun();
                        }
                        catch (Exception ex)
                        {
                            exception = ex;
                        }
                        break;

                    case "u":
                        try
                        {
                            Unregister();
                        }
                        catch (Exception ex)
                        {
                            exception = ex;
                        }
                        break;

                    case "r":
                        try
                        {
                            Register();
                        }
                        catch(Exception ex)
                        {
                            exception = ex;
                        }
                        break;

                    default:
                        WriteLine(ConsoleColor.Red, "Invalid argument: '{0}'", args[0]);
                        wait = true;
                        return;
                }
            }
            
            if (exception != null)
            {
                WriteLine(ConsoleColor.Red, "Error: {0}", exception);
                wait = true;
            }

            if (wait)
                Console.ReadKey(true);
        }

        private static void RegisterAndRun()
        {
            var regRoot = GetRegRoot();
            var configRoot = regRoot.OpenSubKey(ConfigRoot, true);

            if (configRoot == null)
            {
                WriteLine(ConsoleColor.Red,
                          "Registry path '{0}' not found. Microsoft Visual Studio 2008 Shell is not installed.",
                          Path.Combine(regRoot.Name, ConfigRoot));
                return;
            }

            var installDir = configRoot.GetValue("InstallDir") as string;
            if (string.IsNullOrEmpty(installDir))
            {
                WriteLine(ConsoleColor.Red,
                          "Registry value '{0}' not found. Microsoft Visual Studio 2008 Shell is not installed.",
                          Path.Combine(configRoot.Name, "InstallDir"));
                return;
            }

            var fileToRun = Path.Combine(installDir, "devenv.exe");
            if (!File.Exists(fileToRun))
            {
                WriteLine(ConsoleColor.Red,
                          "File '{0}' does not exist. Microsoft Visual Studio 2008 Shell is not installed.",
                          fileToRun);
                return;
            }

            Register(configRoot);

            var process = new Process
                {
                    StartInfo = new ProcessStartInfo(fileToRun)
                        {
                            WorkingDirectory = Environment.CurrentDirectory,
#if DEBUG
                            // Running Experimental Hive
                            Arguments = "/ranu /rootsuffix Exp",
#endif
                        }
                };
            process.Start();
        }

        private static void Register()
        {
            var regRoot = GetRegRoot();
            var configRoot = regRoot.OpenSubKey(ConfigRoot, true);

            Register(configRoot);
        }

        private static void Register(RegistryKey configRoot)
        {
            var type = typeof (CidePackage);
            var context = new ProductRegistrationContext(type, configRoot);

            var attributes = type.GetCustomAttributes(typeof (RegistrationAttribute), true);
            foreach (RegistrationAttribute attr in attributes)
            {
                attr.Register(context);
                Console.WriteLine(@"Ok.");
            }
        }

        private static void Unregister()
        {
            var regRoot = GetRegRoot();
            var configRoot = regRoot.OpenSubKey(ConfigRoot, true);

            Unregister(configRoot);
        }

        private static void Unregister(RegistryKey configRoot)
        {
            var type = typeof(CidePackage);
            var context = new ProductRegistrationContext(type, configRoot);

            var attributes = type.GetCustomAttributes(typeof(RegistrationAttribute), true);
            foreach (RegistrationAttribute attr in attributes)
            {
                var attrName = attr.GetType().Name;
                const string attrSuffix = "Attribute";
                if (attrName.EndsWith(attrSuffix))
                    attrName = attrName.Substring(0, attrName.Length - attrSuffix.Length);
                Console.WriteLine(@"Unregistering {0}...", attrName);
                attr.Unregister(context);
                Console.WriteLine(@"Ok.");
            }
        }

        private static RegistryKey GetRegRoot()
        {
#if RELEASE
                return Registry.LocalMachine;
#else
                return Registry.CurrentUser;
#endif
        }

        private static void WriteLine(ConsoleColor color, string format, params object[] args)
        {
            var c = Console.ForegroundColor;
            try
            {
                Console.ForegroundColor = color;
                if (args == null || args.Length == 0)
                    Console.WriteLine(format);
                else
                    Console.WriteLine(format, args);
            }
            finally
            {
                Console.ForegroundColor = c;
            }
        }
    }
}
