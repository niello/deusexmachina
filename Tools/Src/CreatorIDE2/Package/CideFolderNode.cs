using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using CreatorIDE.Core;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell.Interop;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class CideFolderNode: FolderNode
    {
        private const string GuidString = "F13A5C9F-C62E-4FDA-954F-7537BC28228C";

        public string PackagePath
        {
            get
            {
                var packagePath = ItemNode.GetMetadata(CideProjectElements.FolderPackagePath);
                if (packagePath != null)
                    packagePath = packagePath.Trim();
                
                return !string.IsNullOrEmpty(packagePath) ? packagePath : GetDefaultPackagePath();
            }
            set
            {
                if (value != null)
                    value = value.Trim();

                if (string.IsNullOrEmpty(value))
                    value = null;

                if (value != null && value.ToLowerInvariant() == GetDefaultPackagePath().ToLowerInvariant())
                    value = null;

                string packagePath;
                if (!ItemNode.TryGetMetadata(CideProjectElements.FolderPackagePath, out packagePath))
                {
                    if (value == null)
                        return;
                }
                else if (packagePath == value)
                    return;

                ItemNode.SetMetadata(CideProjectElements.FolderPackagePath, value);
            }
        }
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

        private string GetDefaultPackagePath()
        {
            var parentFolder = Parent as CideFolderNode;
            Debug.Assert(Caption != null);
            return parentFolder != null
                       ? Path.Combine(parentFolder.PackagePath, Caption)
                       : CidePathHelper.AddScope(Configuration.GlobalScope, Caption);
        }
    }

    [ComVisible(true), Guid(GuidString)]
    public class CideFolderNodeProperties: FolderNodeProperties
    {
        private const string GuidString = "54EA3431-FC2E-42AC-8C3F-C6A2266D224B";

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.FullPathPropertyName)]
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

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.PackagePathPropertyName)]
        public string PackagePath
        {
            get { return ((CideFolderNode) Node).PackagePath; }
            set { ((CideFolderNode) Node).PackagePath = value; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.RelativePathPropertyName)]
        public string RelativePath
        {
            get { return Node.VirtualNodeName; }
        }

        public CideFolderNodeProperties(CideFolderNode node):
            base(node)
        {}
    }
}
