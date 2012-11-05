using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;
using IServiceProvider = Microsoft.VisualStudio.OLE.Interop.IServiceProvider;

namespace CreatorIDE.Package
{
    [Guid(GuidString), ComVisible(true)]
    public class CideProjectFactory:ProjectFactory
    {
        private const string GuidString = "953507B5-1A2B-4094-9667-AD34F50110FE";

        internal const string TemplatesDirectory = Configuration.TemplatesDefaultDirectory,
                              ProjectName = "Levels",
                              ProjectExtension = "demcl";

        public CideProjectFactory(CidePackage package):
            base(package)
        {}
               
        
        protected override ProjectNode CreateProject()
        {
            var project = new CideProjectNode((CidePackage) Package);
            project.SetSite((IServiceProvider) ((System.IServiceProvider) Package).GetService(typeof (IServiceProvider)));
            return project;
        }
    }
}
