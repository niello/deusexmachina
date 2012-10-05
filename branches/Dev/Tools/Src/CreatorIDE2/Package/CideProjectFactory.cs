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

        protected override object PreCreateForOuter(IntPtr outerProjectIUnknown)
        {
            // Please be very carefull what is initialized here on the ProjectNode. Normally this should only instantiate and return a project node.
            // The reason why one should very carefully add state to the project node here is that at this point the aggregation has not yet been created and anything that would cause a CCW for the project to be created would cause the aggregation to fail
            // Our reasoning is that there is no other place where state on the project node can be set that is known by the Factory and has to execute before the Load method.
            ProjectNode node = CreateProject();
            Debug.Assert(node != null, "The project failed to be created");
            return node;
        }

        protected override string ProjectTypeGuids(string file)
        {
            return GetType().GUID.ToString("B");
        }
    }
}
