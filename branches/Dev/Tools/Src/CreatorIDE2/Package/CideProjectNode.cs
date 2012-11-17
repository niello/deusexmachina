using System;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.Reflection;
using System.Runtime.InteropServices;
using CreatorIDE.Engine;
using Microsoft.Build.BuildEngine;
using Microsoft.VisualStudio.Project;
using Utilities = Microsoft.VisualStudio.Project.Utilities;
using VsCommands2K = Microsoft.VisualStudio.VSConstants.VSStd2KCmdID;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class CideProjectNode: ProjectNode
    {
        private const string
            GuidString = "42B325C6-0AEB-4f22-B72D-A486015F19E5",
            LanguageID = "11F12F75-2EC0-48e6-96DD-37368172FA18";

        private readonly int _imageListOffset;
        private readonly CidePackage _package;

        public CideEngine Engine { get { return _package.Engine; } }

        public int ImageListOffset { get { return _imageListOffset; } }

        public CideProjectNode(CidePackage package)
        {
            _imageListOffset = ImageHandler.ImageList.Images.Count;
            _package = package;

            InitializeImageList();
        }

        public override Guid ProjectGuid
        {
            get { return typeof (CideProjectNode).GUID; }
        }

        public override string ProjectType
        {
            get { return CideProjectFactory.ProjectName; }
        }

        public override int ImageIndex
        {
            get { return ImageListOffset + Images.WhiteBox; }
        }

        protected override ConfigProvider CreateConfigProvider()
        {
            return new CideConfigProvider(this);
        }

        protected override bool DisableCmdInCurrentMode(Guid commandGroup, uint command)
        {
            if (commandGroup == VsMenus.guidStandardCommandSet2K)
            {
                switch ((VsCommands2K)command)
                {
                    case VsCommands2K.ADDREFERENCE:
                        return true;
                }
            }

            return base.DisableCmdInCurrentMode(commandGroup, command);
        }

        public override Guid GetGuidProperty(VsHPropID propid)
        {
            switch(propid)
            {
                case VsHPropID.PreferredLanguageSID:
                    return new Guid(LanguageID);

                default:
                    return base.GetGuidProperty(propid);
            }
        }

        protected override void ReloadNodes(BuildPropertyGroup projectProperties)
        {
            base.ReloadNodes(projectProperties);

            if (FirstChild == null)
                AddChild(new CideEmptyNode(this));
        }

        protected override void ProcessReferences()
        {
            // References is not supported by the project
        }

        public override void AddChild(HierarchyNode node)
        {
            var emptyNode = FirstChild as CideEmptyNode;

            base.AddChild(node);

            if (emptyNode != null)
                emptyNode.Remove(false);
        }

        public override void RemoveChild(HierarchyNode node)
        {
            base.RemoveChild(node);

            if (FirstChild == null)
                AddChild(new CideEmptyNode(this));
        }

        public override void RemoveAllChildren()
        {
            base.RemoveAllChildren();

            AddChild(new CideEmptyNode(this));
        }

        protected override FolderNode CreateFolderNode(string path, ProjectElement element)
        {
            var folderNode = new CideFolderNode(this, path, element);
            return folderNode;
        }

        public override FileNode CreateFileNode(ProjectElement item)
        {
            var ext = item.GetMetadata(ProjectFileConstants.Include);
            if (ext != null)
            {
                int lastDotIdx = ext.LastIndexOf('.');
                if (lastDotIdx == ext.Length - 1 || lastDotIdx < 0)
                    ext = null;
                else
                    ext = ext.Substring(lastDotIdx + 1);
            }

            if (ext == null)
                return base.CreateFileNode(item);

            switch(ext.ToLower(CultureInfo.InvariantCulture))
            {
                case LevelsNode.FileExtension:
                    return new LevelsNode(this, item);

                default:
                    return base.CreateFileNode(item);
            }
        }

        private void InitializeImageList()
        {
            var asm = Assembly.GetAssembly(typeof (CideProjectNode));
            var resourceStream = asm.GetManifestResourceStream("CreatorIDE.Package.Resources.ImageList24.bmp");
            Debug.Assert(resourceStream != null);
            var customImageList = Utilities.GetImageList(resourceStream);

            foreach (Image img in customImageList.Images)
                ImageHandler.AddImage(img);
        }
    }
}
