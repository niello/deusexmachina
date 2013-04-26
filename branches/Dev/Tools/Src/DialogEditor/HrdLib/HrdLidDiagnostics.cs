using System.IO;

namespace HrdLib
{
    internal static class HrdLidDiagnostics
    {
        public static bool KeepFiles
        {
            get
            {
#if DEBUG
                return true;
#else
                return false;
#endif
            }
        }

        public static string AssemblyGeneratorFolder
        {
            get
            {
                var path = Path.Combine(Path.GetTempPath(), "HrdLibDiagnostics");
                return path;
            }
        }

        public static string TempFileExtension
        {
            get { return ".hrdlib.cs"; }
        }

        public static string AssemblyFileExtension
        {
            get { return ".dll"; }
        }
    }
}
