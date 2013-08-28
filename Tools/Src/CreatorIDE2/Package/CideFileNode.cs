using System;
using System.ComponentModel;
using System.IO;
using CreatorIDE.Core;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell.Interop;

namespace CreatorIDE.Package
{
    public class CideFileNode: FileNode
    {
        public new CideProjectNode ProjectMgr { get { return (CideProjectNode) base.ProjectMgr; } }

        public string Scope
        {
            get
            {
                var folder = Parent as CideFolderNode;
                if (folder != null)
                    return folder.Scope;
                return Configuration.GlobalScope;
            }
        }

        public string PackagePath
        {
            get
            {
                var folder = Parent as CideFolderNode;
                var parentPath = folder == null ? Scope + Configuration.ScopeDelimiterChar : folder.PackagePath;
                return Path.Combine(parentPath, Caption);
            }
        }

        public string RelativePath
        {
            get
            {
                var folder = Parent as CideFolderNode;
                return folder == null || string.IsNullOrEmpty(folder.VirtualNodeName) ? Caption : Path.Combine(folder.VirtualNodeName, Caption);
            }
        }

        public CideBuildAction BuildAction
        {
            get
            {
                var value = ItemNode.GetMetadata(CideProjectElements.FileBuildAction);
                if (!string.IsNullOrEmpty(value))
                {
                    try
                    {
                        return (CideBuildAction) Enum.Parse(typeof (CideBuildAction), value, true);
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
                ItemNode.SetMetadata(CideProjectElements.FileBuildAction, value == CideBuildAction.Inherited ? null : value.ToString());
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

                var folder = Parent as CideFolderNode;
                if (folder != null)
                    return folder.EffectiveBuildAction;

                return CideBuildAction.None;
            }
        }

        public CideFileNode(CideProjectNode project, ProjectElement element):
            base(project, element)
        {
        }

        protected void OnBuildActionChanged()
        {
            ProjectMgr.OnFileBuildActionChanged(this);
        }

        public void Replace(CideFileNode newNode)
        {
            Close();

            Parent.AddChild(newNode);
            RaiseEvent(events => events.OnItemDeleted(ID));
            Parent.RemoveChild(this);
        }

        protected override NodeProperties CreatePropertiesObject()
        {
            return new CideFileNodeProperties(this);
        }
    }

    public class CideFileNodeProperties : FileNodeProperties
    {
        [Browsable(false), AutomationBrowsable(false)]
        public new CideFileNode Node
        {
            get { return (CideFileNode) base.Node; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.BuildActionPropertyName)]
        public new CideBuildAction BuildAction
        {
            get { return Node.BuildAction; }
            set { Node.BuildAction = value; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.EffectiveBuildActionPropertyName)]
        public CideBuildAction EffectiveBuildAction
        {
            get { return Node.EffectiveBuildAction; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.ScopePropertyName)]
        public string Scope
        {
            get { return Node.Scope; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.PackagePathPropertyName)]
        public string PackagePath
        {
            get { return Node.PackagePath; }
        }

        [Browsable(true), AutomationBrowsable(true), SRCategory(SR.GuidString, SR.MiscCategoryName), SRDisplayName(SR.GuidString, SR.RelativePathPropertyName)]
        public string RelativePath
        {
            get { return Node.RelativePath; }
        }

        public CideFileNodeProperties(CideFileNode node)
            : base(node)
        {
        }
    }
}
