using System;

namespace CreatorIDE.Core
{
    internal static class SR
    {
        public const string GuidString = "BD7D4810-548B-431E-AFF6-DBEB09D1050F";
    }

    public class ResourceProvider: ResourceProviderBase
    {
        public ResourceProvider() :
            base(new Guid(SR.GuidString), Resources.ResourceManager)
        {
        }
    }
}
