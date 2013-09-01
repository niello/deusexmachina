using System;
using System.ComponentModel;
using System.IO;
using CreatorIDE.Core;
using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    public class CideFileNode: FileNode, ICideHierarchyNode
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

        ICideHierarchyNode ICideHierarchyNode.Parent
        {
            get { return Parent as ICideHierarchyNode; }
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
            get { return this.GetEffectiveBuildAction(); }
        }

        public string FullPath { get { return Url; } }

        public CideFileNode(CideProjectNode project, ProjectElement element):
            base(project, element)
        {
        }

        void ICideHierarchyNode.OnBuildActionChanged()
        {
            OnBuildActionChanged();
        }

        protected override bool DisableCmdInCurrentMode(Guid commandGroup, uint command)
        {
            if (commandGroup == Commands.ItemNodeMenuGuid)
            {
                var status = this.QueryCommandStatus((CideItemNodeCommand)command);
                return (status & (QueryStatusResult.Supported | QueryStatusResult.Enabled)) !=
                       (QueryStatusResult.Supported | QueryStatusResult.Enabled);
            }
            return base.DisableCmdInCurrentMode(commandGroup, command);
        }

        protected override int QueryStatusOnNode(Guid cmdGroup, uint cmd, IntPtr pCmdText, ref QueryStatusResult result)
        {
            if (cmdGroup == Commands.ItemNodeMenuGuid)
                return ComHelper.WrapFunction(false, this.QueryCommandStatus, (CideItemNodeCommand)cmd, out result);
            return base.QueryStatusOnNode(cmdGroup, cmd, pCmdText, ref result);
        }

        protected override bool ExecCommandOnNode(Guid cmdGroup, uint cmd, uint nCmdexecopt, IntPtr pvaIn, IntPtr pvaOut)
        {
            if (cmdGroup == Commands.ItemNodeMenuGuid)
                return this.ExecuteCommand((CideItemNodeCommand) cmd);
            return base.ExecCommandOnNode(cmdGroup, cmd, nCmdexecopt, pvaIn, pvaOut);
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
