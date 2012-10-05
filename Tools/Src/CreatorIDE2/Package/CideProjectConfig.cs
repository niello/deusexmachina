using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    public class CideProjectConfig:ProjectConfig
    {
        public CideProjectConfig(CideProjectNode project, string configuration):
            base(project, configuration)
        {
        }

        protected override bool CanLaunchDebug(uint flags)
        {
            // Project is not debuggable
            return false;
        }
    }
}
