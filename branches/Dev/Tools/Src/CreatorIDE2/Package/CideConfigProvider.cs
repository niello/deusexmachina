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

        protected override bool IsConfigSupported(VsConfigPropID propID)
        {
            return false;
        }

        protected override void GetConfigs(uint celt, IVsCfg[] a, uint[] actual, uint[] flags)
        {
            if (flags != null)
                flags[0] = 0;

            int i = 0;
            string[] configList = new[] {"Release"};

            if (a != null)
            {
                foreach (string configName in configList)
                {
                    a[i] = GetProjectConfiguration(configName);

                    i++;
                    if (i == celt)
                        break;
                }
            }
            else
                i = configList.Length;

            if (actual != null)
                actual[0] = (uint)i;
        }

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
