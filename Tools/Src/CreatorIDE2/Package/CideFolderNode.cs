using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class CideFolderNode:FolderNode
    {
        private const string GuidString = "F13A5C9F-C62E-4FDA-954F-7537BC28228C";

        public CideFolderNode(CideProjectNode root, string relativePath, ProjectElement element):
            base(root, relativePath, element)
        {
            
        }

        protected override NodeProperties CreatePropertiesObject()
        {
            return new CideFolderNodeProperties(this);
        }
    }

    [ComVisible(true), Guid(GuidString)]
    public class CideFolderNodeProperties:FolderNodeProperties
    {
        private const string GuidString = "54EA3431-FC2E-42AC-8C3F-C6A2266D224B";

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.MiscCategoryName), SRDisplayName(SR.FullPathPropertyName)]
        public new string FullPath
        {
            get
            {
                var fullPath = base.FullPath;
                if (!string.IsNullOrEmpty(fullPath))
                    fullPath = fullPath.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
                return fullPath;
            }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.MiscCategoryName), SRDisplayName(SR.RelativePathPropertyName)]
        public string RelativePath
        {
            get { return Node.VirtualNodeName; }
        }

        public CideFolderNodeProperties(CideFolderNode node):
            base(node)
        {}
    }
}
