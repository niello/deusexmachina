using System;
using System.Resources;
using CreatorIDE.Core;

namespace CreatorIDE.Package
{
    internal sealed class SR: StringResources<ResourceProvider>
    {
        public const string
            GuidString = "2C77FF92-B8B4-4653-8F4E-5C5AC52895AE";

        public const string
            BuildActionPropertyName = "BuildActionPropertyName",
            EffectiveBuildActionPropertyName = "EffectiveBuildActionPropertyName",
            EmptyNodeCaption = "EmptyNodeCaption",
            FileNotFound = "FileNotFound",
            FileNotFoundFormatString = "FileNotFoundFormatString",
            FolderBuildActionPropertyName = "BuildActionPropertyName",
            FullPathPropertyName = "FullPathPropertyName",
            LevelDatabaseFile = "LevelDatabaseFile",
            LevelObjectBrowserCaption = "LevelObjectBrowserCaption",
            MiscCategoryName = "MiscCategoryName",
            OpenLevelDatabaseFile = "OpenLevelDatabaseFile",
            PackagePathPropertyName = "PackagePathPropertyName",
            RelativePathPropertyName = "RelativePathPropertyName",
            ScopeIsAlreadyDeclaredFormatString = "ScopeIsAlreadyDeclaredFormatString",
            ScopeNameIsInvalidFormatString = "ScopeNameIsInvalidFormatString",
            ScopeNameIsTooShort = "ScopeNameIsTooShort",
            ScopePropertyName = "ScopePropertyName";

        private SR()
        {
        }
    }

    public class ResourceProvider : IResourceProvider
    {
        public static ResourceProvider Instance { get { return SR.Provider; } }

        public Guid TypeID { get { return new Guid(SR.GuidString); } }

        public ResourceManager ResourceManager { get { return Resources.ResourceManager; } }
    }
}
