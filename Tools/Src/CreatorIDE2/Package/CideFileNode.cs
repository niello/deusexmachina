using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    public class CideFileNode: FileNode
    {
        public new CideProjectNode ProjectMgr { get { return (CideProjectNode) base.ProjectMgr; } }

        public CideFileNode(CideProjectNode project, ProjectElement element):
            base(project, element)
        {
        }
    }
}
