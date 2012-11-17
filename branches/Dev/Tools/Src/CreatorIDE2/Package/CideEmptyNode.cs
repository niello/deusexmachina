using System;
using System.Globalization;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class CideEmptyNode: HierarchyNode
    {
        public const string GuidString = "50D039D2-16F5-4DD5-8CC3-ECDB72854460";
        
        public override string Url
        {
            get { return string.Empty; }
        }

        public override string Caption
        {
            get { return SR.GetString(SR.EmptyNodeCaption, CultureInfo.CurrentCulture); }
        }

        public override Guid ItemTypeGuid
        {
            get { return typeof (CideEmptyNode).GUID; }
        }

        public override int ImageIndex
        {
            get { return ((CideProjectNode)ProjectMgr).ImageListOffset + Images.Transparent; }
        }

        public override bool CanExecuteCommand
        {
            get { return false; }
        }

        public CideEmptyNode(CideProjectNode project):
            base(project)
        {
        }

        public override string GetEditLabel()
        {
            return null;
        }
    }
}
