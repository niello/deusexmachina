using System;

namespace CreatorIDE.Core
{
    public static class ServiceProviderExtension
    {
        public static T GetService<T>(this IServiceProvider provider)
            where T:class
        {
            return provider.GetService<T>(typeof (T));
        }

        public static T GetService<T>(this IServiceProvider provider, Type serviceType)
            where T:class
        {
            if (serviceType == null)
                throw new ArgumentException("serviceType");

            if (provider == null)
                return null;

            return provider.GetService(serviceType) as T;
        }
    }
}
