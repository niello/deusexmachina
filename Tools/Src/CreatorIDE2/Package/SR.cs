using System.Globalization;

namespace CreatorIDE.Package
{
    internal static class SR
    {
        public const string FileNotFound = "FileNotFound",
                            FileNotFoundFormatString = "FileNotFoundFormatString",
                            FullPathPropertyName = "FullPathPropertyName",
                            LevelDatabaseFile = "LevelDatabaseFile",
                            MiscCategoryName = "MiscCategoryName",
                            OpenLevelDatabaseFile = "OpenLevelDatabaseFile",
                            RelativePathPropertyName = "RelativePathPropertyName";

        public static string GetString(string name, CultureInfo cultureInfo)
        {
            return Resources.ResourceManager.GetString(name, cultureInfo);
        }

        public static string GetString(string name)
        {
            return Resources.ResourceManager.GetString(name, CultureInfo.CurrentCulture);
        }
    }
}
