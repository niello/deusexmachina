using System;
using System.Resources;
using CreatorIDE.Core;

namespace CreatorIDE.Engine
{
    internal sealed class SR: StringResources<ResourceProvider>
    {
        public const string GuidString = "37DC84A8-E2F7-4108-B52B-BABC0A43DC5D";

        public const string CategoryWithNameExistsFormat = "CategoryWithNameExistsFormat",
                            EmptyCategoryNameDisallowed = "EmptyCategoryNameDisallowed",
                            EngineInitFailFormat = "EngineInitFailFormat",
                            NewEntityDefaultName = "NewEntityDefaultName",
                            ValueNotSupportedFormat = "ValueNotSupportedFormat";

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
