using System;
using System.IO;
using System.Reflection;

namespace DialogDesigner
{
    public static class ProgramEnvironment
    {
        public static readonly string AppDirectory, ExecutableFile;

        private static ProgramConfiguration _configuration;
        public static ProgramConfiguration Configuration
        {
            get
            {
                if (_configuration == null)
                    _configuration = ProgramConfiguration.Load();
                return _configuration;
            }
        }

        static ProgramEnvironment()
        {
            var asm = Assembly.GetAssembly(typeof (Program));
            ExecutableFile = asm.Location;
            AppDirectory = Path.GetDirectoryName(ExecutableFile);
        }

        public static void SaveConfiguration()
        {
            try
            {
                if(Configuration!=null)
                    Configuration.Save();
            }
            catch(Exception ex)
            {
                Console.WriteLine(@"Can't save configuration: {0}", ex.Message);
            }
        }
    }
}
