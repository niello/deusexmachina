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
    public class CideFolderNode: FolderNode, ICideHierarchyNode
    {
        private const string GuidString = "F13A5C9F-C62E-4FDA-954F-7537BC28228C";

        public new CideProjectNode ProjectMgr { get { return (CideProjectNode) base.ProjectMgr; } }

        public override int ImageIndex
        {
            get { return ProjectMgr.ImageListOffset + (IsScopeRoot ? Images.FolderArrowGreen : Images.Folder); }
        }

        public string PackagePath
        {
            get
            {
                if (IsScopeRoot)
                    return Scope + CidePathHelper.ScopeSeparatorChar;

                var parent = Parent as CideFolderNode;
                string parentPath = parent == null
                                        ? Configuration.GlobalScope + CidePathHelper.ScopeSeparatorChar
                                        : parent.PackagePath;
                return Path.Combine(parentPath, Caption);
            }
        }

        public string Scope
        {
            get
            {
                var scope = ItemNode.GetMetadata(CideProjectElements.FolderScope);
                if (scope != null)
                    scope = scope.Trim().ToLowerInvariant();

                return string.IsNullOrEmpty(scope) ? GetDefaultScope() : scope;
            }
            set
            {
                if (value != null)
                    value = value.Trim().ToLowerInvariant();

                if (string.IsNullOrEmpty(value) || value == GetDefaultScope())
                    value = null;

                string currentScope;
                if (ItemNode.TryGetMetadata(CideProjectElements.FolderScope, out currentScope) && currentScope != null)
                {
                    currentScope = currentScope.Trim().ToLowerInvariant();
                    if (string.IsNullOrEmpty(currentScope))
                        currentScope = null;
                }

                if (currentScope == value)
                    return;

                if (currentScope != null && !string.IsNullOrEmpty(currentScope.Trim().ToLowerInvariant()))
                {
                    bool removed = ProjectMgr.RemoveFromScopeMap(this);
                    Debug.Assert(removed);
                }

                if (value != null)
                    ProjectMgr.AddToScopeMap(value, this);

                ItemNode.SetMetadata(CideProjectElements.FolderScope, value);

                OnScopeChanged();
            }
        }

        public bool IsScopeRoot
        {
            get
            {
                string scope;
                return ItemNode.TryGetMetadata(CideProjectElements.FolderScope, out scope) && scope != null &&
                       !string.IsNullOrEmpty(scope.Trim());
            }
        }

        public CideBuildAction BuildAction
        {
            get
            {
                var buildAction = ItemNode.GetMetadata(CideProjectElements.FolderBuildAction);
                if (!string.IsNullOrEmpty(buildAction))
                {
                    try
                    {
                        return (CideBuildAction) Enum.Parse(typeof (CideBuildAction), buildAction, true);
                    }
                    catch (ArgumentException)
                    {
                    }
                }
                return CideBuildAction.Inherited;
            }
            set
            {
                var oldAction = EffectiveBuildAction;
                ItemNode.SetMetadata(CideProjectElements.FolderBuildAction, value == CideBuildAction.Inherited ? null : value.ToString());
                if (oldAction != EffectiveBuildAction)
                    OnBuildActionChanged();
            }
        }

        public CideBuildAction EffectiveBuildAction
        {
            get
            {
                var action = BuildAction;
                if (action != CideBuildAction.Inherited)
                    return action;

                var parent = Parent as CideFolderNode;
                if (parent != null)
                    return parent.EffectiveBuildAction;

                return CideBuildAction.None;
            }
        }

        public CideFolderNode(CideProjectNode root, string relativePath, ProjectElement element):
            base(root, relativePath, element)
        {
        }

        void ICideHierarchyNode.OnBuildActionChanged()
        {
            OnBuildActionChanged();
        }

        private void OnScopeChanged()
        {
            ReDraw(UIHierarchyElement.Icon | UIHierarchyElement.Caption);
        }

        private void OnBuildActionChanged()
        {
            for (var child = FirstChild; child != null; child = child.NextSibling)
            {
                var cideHier = child as ICideHierarchyNode;
                if (cideHier == null)
                    continue;

                if (cideHier.BuildAction == CideBuildAction.Inherited)
                    cideHier.OnBuildActionChanged();
            }
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

        private string GetDefaultScope()
        {
            var parentFolder = Parent as CideFolderNode;
            return parentFolder == null ? Configuration.GlobalScope : parentFolder.Scope;
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
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.RelativePathPropertyName)]
        public string RelativePath
        {
            get { return Node.VirtualNodeName; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.ScopePropertyName)]
        public string Scope
        {
            get { return ((CideFolderNode) Node).Scope; }
            set { ((CideFolderNode) Node).Scope = value; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.FolderBuildActionPropertyName)]
        public CideBuildAction BuildAction
        {
            get { return ((CideFolderNode) Node).BuildAction; }
            set { ((CideFolderNode) Node).BuildAction = value; }
        }

        public CideFolderNodeProperties(CideFolderNode node):
            base(node)
        {}
    }
}
