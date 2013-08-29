using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Globalization;
using System.IO;
using System.Reflection;
using System.Resources;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using CreatorIDE.Engine;
using Microsoft.Build.BuildEngine;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell;
using VsCommands2K = Microsoft.VisualStudio.VSConstants.VSStd2KCmdID;
using VsMenus = Microsoft.VisualStudio.Project.VsMenus;

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
        private readonly Dictionary<string, CideFolderNode> _scopeMap = new Dictionary<string, CideFolderNode>();

        public CideEngine Engine { get { return _package.Engine; } }

        public int ImageListOffset { get { return _imageListOffset; } }

        public sealed override Guid ProjectIDGuid
        {
            get { return base.ProjectIDGuid; }
            set
            {
                if (ProjectIDGuid == value || !IsProjectOpened)
                    base.ProjectIDGuid = value;
                else
                {
                    // Each project registers itself in the package
                    _package.UnregisterProject(this);
                    base.ProjectIDGuid = value;
                    _package.RegisterProject(this);
                }
            }
        }

        public CideProjectNode(CidePackage package)
        {
            _imageListOffset = ImageHandler.ImageList.Images.Count;
            _package = package;

            InitializeImageList();
        }

        public new CidePackage Package { get { return (CidePackage)base.Package; } protected set { base.Package = value; } }

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
            if (folderNode.IsScopeRoot)
                AddToScopeMap(folderNode.Scope, folderNode);
            return folderNode;
        }

        protected override ProjectElement AddFileToMsBuild(string file)
        {
            var fileExtension = Path.GetExtension(file);
            if (fileExtension != null)
                fileExtension = fileExtension.ToLowerInvariant();

            CideBuildAction buildAction;
            switch (fileExtension)
            {
                case ".hrd":
                    buildAction = CideBuildAction.Inherited;
                    break;

                case ".dlg":
                    buildAction = CideBuildAction.Dialogue;
                    break;

                case ".lua":
                    buildAction = CideBuildAction.LuaCode;
                    break;

                case null:
                case "":
                    buildAction = CideBuildAction.None;
                    break;

                default:
                    buildAction = CideBuildAction.Content;
                    break;
            }

            var itemPath = PackageUtilities.MakeRelativeIfRooted(file, BaseURI);
            Debug.Assert(!Path.IsPathRooted(itemPath), "Cannot add item with full path.");

            // All files in the project are content files. Build action is managed by SubType property
            var newItem = CreateMsBuildFileItem(itemPath, ProjectFileConstants.Content);
            if (buildAction != CideBuildAction.Inherited)
                newItem.SetMetadata(CideProjectElements.FileBuildAction, buildAction.ToString());
            return newItem;
        }

        public override FileNode CreateFileNode(ProjectElement item, HierarchyNode parentNode)
        {
            var buildAction = CideBuildAction.Inherited;
            var buildActionStr = item.GetMetadata(CideProjectElements.FileBuildAction);
            if (!string.IsNullOrEmpty(buildActionStr))
            {
                try
                {
                    buildAction = (CideBuildAction) Enum.Parse(typeof (CideBuildAction), buildActionStr, true);
                }
                catch (ArgumentException)
                {
                }
            }
            if (buildAction == CideBuildAction.Inherited)
            {
                var folder = parentNode as CideFolderNode;
                if (folder != null)
                    buildAction = folder.EffectiveBuildAction;
            }

            switch (buildAction)
            {
                case CideBuildAction.Level:
                    return new LevelNode(this, item);

                default:
                    return new CideFileNode(this, item);
            }
        }

        protected override void OnAfterProjectOpen(object sender, AfterProjectFileOpenedEventArgs e)
        {
            Debug.Assert(ProjectIDGuid != Guid.Empty);
            _package.RegisterProject(this);

            base.OnAfterProjectOpen(sender, e);
        }

        protected override void OnBeforeProjectClosed(object sender, BeforeProjectFileClosedEventArgs e)
        {
            base.OnBeforeProjectClosed(sender, e);

            _package.UnregisterProject(this);
        }

        private void InitializeImageList()
        {
            var asm = Assembly.GetAssembly(typeof (CideProjectNode));
            var resourceManager = new ResourceManager("CreatorIDE.Package.VSPackage", asm);
            var bitmap = resourceManager.GetObject(Images.ImageStripResID.ToString(CultureInfo.InvariantCulture)) as Bitmap;
            Debug.Assert(bitmap != null);

            var customImageList = new ImageList
                                      {
                                          ColorDepth = ColorDepth.Depth24Bit,
                                          ImageSize = new Size(16, 16),
                                          TransparentColor = Color.Magenta
                                      };

            customImageList.Images.AddStrip(bitmap);

            foreach (Image img in customImageList.Images)
                ImageHandler.AddImage(img);
        }

        public bool RemoveFromScopeMap(CideFolderNode folder)
        {
            if (folder == null)
                return false;

            if (!folder.IsScopeRoot)
                return false;

#if DEBUG
            CideFolderNode originalFolder;
            if (_scopeMap.TryGetValue(folder.Scope, out originalFolder))
                Debug.Assert(ReferenceEquals(folder, originalFolder));
#endif

            return _scopeMap.Remove(folder.Scope);
        }

        public void AddToScopeMap(string scope, CideFolderNode folder)
        {
            if (scope == null)
                throw new ArgumentNullException("scope");
            if(folder==null)
                throw new ArgumentException("folder");
            if (scope.Length < 2)
                throw new ArgumentException(SR.GetString(SR.ScopeNameIsTooShort), "scope");
            if (scope == Configuration.GlobalScope && !CidePathHelper.IsValidScopeName(scope))
                throw new ArgumentException(SR.GetFormatString(SR.ScopeNameIsInvalidFormatString, scope), "scope");

            CideFolderNode exisitngNode;
            if (_scopeMap.TryGetValue(scope, out exisitngNode))
            {
                if (ReferenceEquals(exisitngNode, folder))
                    return;

                throw new InvalidOperationException(SR.GetFormatString(SR.ScopeIsAlreadyDeclaredFormatString, scope,
                                                                       exisitngNode.VirtualNodeName));
            }

            _scopeMap.Add(scope, folder);
        }

        public bool TryGetFromScopeMap(string scope, out CideFolderNode folder)
        {
            if (scope == null)
            {
                folder = null;
                return false;
            }
            return _scopeMap.TryGetValue(scope, out folder);
        }

        public void OnFileBuildActionChanged(CideFileNode cideFileNode)
        {
            switch (cideFileNode.EffectiveBuildAction)
            {
                case CideBuildAction.Level:
                    if (cideFileNode is LevelNode)
                        return;
                    break;

                default:
                    if (cideFileNode.GetType() == typeof (CideFileNode))
                        return;
                    break;
            }

            var newNode = (CideFileNode) CreateFileNode(cideFileNode.ItemNode, cideFileNode.Parent);
            cideFileNode.Replace(newNode);
        }
    }
}
