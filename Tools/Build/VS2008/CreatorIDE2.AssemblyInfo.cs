#if SLN_CREATORIDE2
using System.Reflection;

[assembly: AssemblyConfiguration(CreatorIDE.Settings.Properties.Configuration)]
[assembly: AssemblyCompany(CreatorIDE.Settings.Properties.Company)]
[assembly: AssemblyProduct(CreatorIDE.Settings.Properties.Product)]
[assembly: AssemblyCopyright(CreatorIDE.Settings.Properties.Copyright)]
[assembly: AssemblyTrademark(CreatorIDE.Settings.Properties.Trademark)]

namespace CreatorIDE.Settings
{
    static partial class Properties
    {
#if DEBUG
        public const string Configuration = "Debug";
#elif RELEASE
	    public const string Configuration = "Release";
#endif
#if !OVERRIDE_PRODUCT_INFO
        public const string
            Company = "Still No Team Name",
            Product = "CreatorIDE 2",
            Copyright = "(c) 2011 - 2013",
            Trademark = Product;
#endif
    }
}
#endif