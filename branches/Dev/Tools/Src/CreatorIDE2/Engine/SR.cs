using System;
using System.Globalization;
using CreatorIDE.Core;

namespace CreatorIDE.Engine
{
    internal static class SR
    {
        public const string GuidString = "37DC84A8-E2F7-4108-B52B-BABC0A43DC5D";

        public const string EngineInitFailFormat = "EngineInitFailFormat";

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
