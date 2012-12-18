using System.IO;
using System.Reflection;

namespace CreatorIDE.Package
{
    internal static class Configuration
    {
        public const string
            TemplatesDefaultDirectory = @"..\..\Src\CreatorIDE2\Package\Templates",
            GlobalScope = "global",
            HomeScope = "home",
            ProjectScope = "proj";

        public const char ScopeDelimiterChar = ':';

        public static readonly string AppFolder;

        static Configuration()
        {
            var asm = Assembly.GetAssembly(typeof (Configuration));
            AppFolder = Path.GetDirectoryName(asm.Location) ?? string.Empty;
        }
    }
}
