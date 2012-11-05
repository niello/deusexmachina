using System;
using System.Diagnostics;
using System.Drawing;
using System.Reflection;
using System.Runtime.InteropServices;
using CreatorIDE.Engine;
using Microsoft.VisualStudio.Project;

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

        public override int QueryStatusCommand(uint itemId, ref Guid guidCmdGroup, uint cCmds, Microsoft.VisualStudio.OLE.Interop.OLECMD[] cmds, IntPtr pCmdText)
        {
            var node = ProjectMgr.NodeFromItemId(itemId);
            if (node is LevelsNode)
                return node.QueryStatusCommand(itemId, ref guidCmdGroup, cCmds, cmds, pCmdText);
            return base.QueryStatusCommand(itemId, ref guidCmdGroup, cCmds, cmds, pCmdText);
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
