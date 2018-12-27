using System;
using System.IO;
using System.Xml.Serialization;

namespace DialogDesigner
{
    public class ProgramConfiguration
    {
        public string DefaultDirectory { get; set; }
        public string RecentProjectFile { get; set; }

        [Obsolete("For serialization usage only!", true)]
        public ProgramConfiguration()
        {}

        private ProgramConfiguration(string defaultDirectory)
        {
            DefaultDirectory = defaultDirectory;
        }

        public static ProgramConfiguration Load()
        {
            try
            {
                var path = Path.Combine(ProgramEnvironment.AppDirectory, "DialogEditor.xml");
                if (!File.Exists(path))
                    return new ProgramConfiguration(ProgramEnvironment.AppDirectory);

                var serializer = new XmlSerializer(typeof (ProgramConfiguration));
                ProgramConfiguration result;
                using (var fStream = new FileStream(path, FileMode.Open, FileAccess.Read))
                    result = (ProgramConfiguration) serializer.Deserialize(fStream);

                result.DefaultDirectory = GetFullPath(result.DefaultDirectory);
                result.RecentProjectFile = GetFullPath(result.RecentProjectFile);
                return result;
            }
            catch
            {
                return new ProgramConfiguration(ProgramEnvironment.AppDirectory);
            }
        }

        public void Save()
        {
            var serializedCopy = new ProgramConfiguration(GetRelativePath(DefaultDirectory))
                                     {RecentProjectFile = GetRelativePath(RecentProjectFile)};

            var path = Path.Combine(ProgramEnvironment.AppDirectory, "DialogEditor.xml");
            var serializer = new XmlSerializer(typeof (ProgramConfiguration));
            using (var fStream = new FileStream(path, FileMode.Create, FileAccess.Write))
                serializer.Serialize(fStream, serializedCopy);
        }

        private static string GetFullPath(string path)
        {
            if(path==null || Path.IsPathRooted(path))
                return path;

            return Path.GetFullPath(Path.Combine(ProgramEnvironment.AppDirectory, path));
        }

        private static string GetRelativePath(string path)
        {
            if(path==null || !Path.IsPathRooted(path))
                return path;

            return PathHelper.GetRelativePath(ProgramEnvironment.AppDirectory, path);
        }
    }
}
