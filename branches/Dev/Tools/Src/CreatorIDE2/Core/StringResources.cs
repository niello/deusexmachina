using System.Globalization;
using System.Resources;

namespace CreatorIDE.Core
{
    /// <summary>
    /// Provides basic methods for the access to string resources.
    /// </summary>
    /// <typeparam name="TProvider">Class which provides access to the internal resources of the assembly.</typeparam>
    /// <remarks> 
    /// This class is designed like a static one, so it doesn't contain any non-static method (except constructor).
    /// It's recommendend to make derived classes the same way to avoid errors.
    /// </remarks>
    public abstract class StringResources<TProvider>
        where TProvider : IResourceProvider, new()
    {
        public static readonly TProvider Provider;
        private static readonly ResourceManager ResourceManager;

        static StringResources()
        {
            Provider = new TProvider();
            ResourceManager = Provider.ResourceManager;
        }

        public static string GetString(string name)
        {
            return ResourceManager.GetString(name, CultureInfo.CurrentCulture);
        }

        public static string GetString(CultureInfo cultureInfo, string name)
        {
            return ResourceManager.GetString(name, cultureInfo);
        }

        public static string GetFormatString(string name, params object[] args)
        {
            var formatString = ResourceManager.GetString(name, CultureInfo.CurrentCulture);
            return formatString == null ? null : string.Format(formatString, args);
        }

        public static string GetFormatString(CultureInfo cultureInfo, string name, params object[] args)
        {
            var formatString = ResourceManager.GetString(name, cultureInfo);
            return formatString == null ? null : string.Format(formatString, args);
        }
    }
}
