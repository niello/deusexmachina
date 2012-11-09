using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell.Interop;

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

        protected override void AddItemToHierarchy(HierarchyAddType addType)
        {
            string strFilter = String.Empty;
            int iDontShowAgain;
            VsAddItemFlags uiFlags;

            var project = ProjectMgr;

            var strBrowseLocations = GetMkDocument();

            var projectGuid = ProjectMgr.ProjectGuid;

            var addItemDialog = (IVsAddProjectItemDlg)GetService(typeof(IVsAddProjectItemDlg));

            if (addType == HierarchyAddType.AddNewItem)
                uiFlags = VsAddItemFlags.AddNewItems | VsAddItemFlags.SuggestTemplateName | VsAddItemFlags.AllowHiddenTreeView;
            else
                uiFlags = VsAddItemFlags.AddExistingItems | VsAddItemFlags.AllowMultiSelect | VsAddItemFlags.ShowLocationField;

            ErrorHandler.ThrowOnFailure(addItemDialog.AddProjectItemDlg(ID, ref projectGuid, project, (uint) uiFlags, strBrowseLocations, null, ref strBrowseLocations, ref strFilter, out iDontShowAgain));
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
