using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell.Interop;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class CideConfigProvider:ConfigProvider
    {
        private const string GuidString = "81C020F9-BE1B-4afb-BF5B-70651EA2C376";

        private static readonly string[] SupportedPlatforms = new[] {"AnyPlatform"};

        public CideConfigProvider(CideProjectNode manager):
            base(manager)
        {}

        protected override void GetPlatforms(uint celt, string[] names, uint[] actual)
        {
            GetPlatforms(celt, names, actual, SupportedPlatforms);
        }

        protected override void GetSupportedPlatforms(uint celt, string[] names, uint[] actual)
        {
            GetPlatforms(celt, names, actual, SupportedPlatforms);
        }

        protected override ProjectConfig CreateProjectConfiguration(string configName)
        {
            return new CideProjectConfig((CideProjectNode) ProjectMgr, configName);
        }
    }
}
