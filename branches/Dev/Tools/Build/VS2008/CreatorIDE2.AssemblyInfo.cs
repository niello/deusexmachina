using System.Reflection;

[assembly: AssemblyConfiguration(CreatorIDE.Settings.Properties.Configuration)]
[assembly: AssemblyCompany(CreatorIDE.Settings.Properties.Company)]
[assembly: AssemblyProduct(CreatorIDE.Settings.Properties.Product)]
[assembly: AssemblyCopyright(CreatorIDE.Settings.Properties.Copyright)]
//[assembly: AssemblyTrademark("")]
//[assembly: AssemblyCulture("")]

namespace CreatorIDE.Settings
{
	static partial class Properties
	{
		public const string 
			Configuration =
#if DEBUG
				"Debug"
#elif RELEASE
				"Release"
#else
				"Undefined"
#endif
#if !OVERRIDE_PRODUCT_INFO
			, Company = "Still No Team Name"
			, Product = "DemCreator"
			, Copyright = "(c) 2011 - 2012"
#endif
			;
	}
}