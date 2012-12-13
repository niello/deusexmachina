using System;
using System.Globalization;
using CreatorIDE.Core;

namespace CreatorIDE.Package
{
    internal static class SR
    {
        public const string GuidString = "2C77FF92-B8B4-4653-8F4E-5C5AC52895AE";

        public const string EmptyNodeCaption = "EmptyNodeCaption",
                            FileNotFound = "FileNotFound",
                            FileNotFoundFormatString = "FileNotFoundFormatString",
                            FullPathPropertyName = "FullPathPropertyName",
                            LevelDatabaseFile = "LevelDatabaseFile",
                            MiscCategoryName = "MiscCategoryName",
                            OpenLevelDatabaseFile = "OpenLevelDatabaseFile",
                            PackagePathPropertyName = "PackagePathPropertyName",
                            RelativePathPropertyName = "RelativePathPropertyName",
                            ScopeNameIsTooShort = "ScopeNameIsTooShort";

        public static string GetString(string name, CultureInfo cultureInfo)
        {
            return Resources.ResourceManager.GetString(name, cultureInfo);
        }

        public static string GetString(string name)
        {
            return Resources.ResourceManager.GetString(name, CultureInfo.CurrentCulture);
        }
    }

    public class ResourceProvider : ResourceProviderBase
    {
        public ResourceProvider() :
            base(new Guid(SR.GuidString), Resources.ResourceManager)
        {
        }
    }
}
