using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Text;
using CreatorIDE.Core;
using Microsoft.VisualStudio.Project;

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

        public CideFileNode(CideProjectNode project, ProjectElement element):
            base(project, element)
        {
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
            get
            {
                var value = Node.ItemNode.ItemName;
                if (string.IsNullOrEmpty(value))
                    return CideBuildAction.None;
                
                try
                {
                    return (CideBuildAction) Enum.Parse(typeof (CideBuildAction), value, true);
                }
                catch (ArgumentException)
                {
                    return CideBuildAction.None;
                }
            }
            set { Node.ItemNode.ItemName = value.ToString(); }
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
