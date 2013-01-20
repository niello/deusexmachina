using System;
using System.Resources;

namespace CreatorIDE.Core
{
    internal sealed class SR: StringResources<ResourceProvider>
    {
        public const string GuidString = "BD7D4810-548B-431E-AFF6-DBEB09D1050F";

        public const string
            CircularBufferEmpty = "CircularBufferEmpty",
            CircularBufferInvalidCapacity = "CircularBufferInvalidCapacity",
            CircularBufferInvalidPosition = "CircularBufferInvalidPosition";

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
