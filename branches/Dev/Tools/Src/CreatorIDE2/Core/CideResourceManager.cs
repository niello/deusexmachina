using System;
using System.Collections.Generic;
using System.Globalization;
using System.Resources;

namespace CreatorIDE.Core
{
    public static class CideResourceManager
    {
        private static readonly Dictionary<Guid, ResourceProviderBase> Providers =
            new Dictionary<Guid, ResourceProviderBase>();

        public static void RegisterProvider(ResourceProviderBase provider)
        {
            if (provider == null)
                throw new ArgumentNullException("provider");
            Providers.Add(provider.TypeID, provider);
        }

        public static string GetString(Guid typeID, string name, CultureInfo cultureInfo)
        {
            var mgr = GetManager(typeID);
            var res = mgr == null ? null : mgr.GetString(name, cultureInfo);
            return res;
        }

        public static string GetString(Guid typeID, string name)
        {
            var mgr = GetManager(typeID);
            var res = mgr == null ? null : mgr.GetString(name);
            return res;
        }

        private static ResourceManager GetManager(Guid typeID)
        {
            ResourceProviderBase provider;
            var mgr = Providers.TryGetValue(typeID, out provider) ? provider.ResourceManager : null;
            return mgr;
        }
    }
}
