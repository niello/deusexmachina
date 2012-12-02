using System.Reflection;

namespace CreatorIDE.Settings
{
	[assembly: AssemblyConfiguration(Properties.Configuration)]
	[assembly: AssemblyCompany(Properties.Company)]
	[assembly: AssemblyProduct(Properties.Product)]
	[assembly: AssemblyCopyright(Properties.Copyright)]
	//[assembly: AssemblyTrademark("")]
	//[assembly: AssemblyCulture("")]
	
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
			, Product = "CreatorIDE"
			, Copyright = "(c) 2011 - 2012"
#endif
			;
	}
}