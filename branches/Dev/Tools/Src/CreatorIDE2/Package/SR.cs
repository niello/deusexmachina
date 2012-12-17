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
                            ScopeIsAlreadyDeclaredFormatString = "ScopeIsAlreadyDeclaredFormatString",
                            ScopeNameIsInvalidFormatString = "ScopeNameIsInvalidFormatString",
                            ScopeNameIsTooShort = "ScopeNameIsTooShort",
                            ScopePropertyName = "ScopePropertyName";

        public static string GetFormatString(CultureInfo cultureInfo, string name, params object[] args)
        {
            var formatString = Resources.ResourceManager.GetString(name, cultureInfo);
            return formatString == null ? null : string.Format(formatString, args);
        }

        public static string GetString(CultureInfo cultureInfo, string name)
        {
            return Resources.ResourceManager.GetString(name, cultureInfo);
        }

        public static string GetFormatString(string name, params object[] args)
        {
            var formatString = Resources.ResourceManager.GetString(name);
            return formatString == null ? null : string.Format(formatString, args);
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
