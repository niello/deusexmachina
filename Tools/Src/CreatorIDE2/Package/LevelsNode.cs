using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell.Interop;
using Constants = Microsoft.VisualStudio.OLE.Interop.Constants;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class LevelsNode: HierarchyNode
    {
        private const string GuidString = "0CEA25A5-4799-4461-A7FB-79C77EA04743";
        private const string DbFileExtension = "db3";

        private string _path;

        public LevelsNode(string path, CideProjectNode projectNode):
            base(projectNode)
        {
            _path = path;
            VirtualNodeName = "Levels";
        }

        public override string Url
        {
            get { return VirtualNodeName; }
        }

        public override string Caption
        {
            get { return "Levels"; }
        }

        public override Guid ItemTypeGuid
        {
            get { return typeof (LevelsNode).GUID; }
        }

        public override int ImageIndex
        {
            get { return ((CideProjectNode) ProjectMgr).ImageListOffset + Images.Globe; }
        }

        public override int MenuCommandId
        {
            get { return VsMenus.IDM_VS_CTXT_ITEMNODE; }
        }

        public override string GetEditLabel()
        {
            return null;
        }

        protected override bool CanDeleteItem(__VSDELETEITEMOPERATION deleteOperation)
        {
            return false;
        }

        protected override bool DisableCmdInCurrentMode(Guid commandGroup, uint command)
        {
            if(commandGroup==Commands.LevelsNodeMenuGuid)
                return false;
            return base.DisableCmdInCurrentMode(commandGroup, command);
        }

        protected override int ExecCommandOnNode(Guid cmdGroup, uint cmd, uint nCmdexecopt, IntPtr pvaIn, IntPtr pvaOut)
        {
            if (cmdGroup == Commands.LevelsNodeMenuGuid)
                return ComHelper.WrapAction(false, ExecNodeCommand, cmd);
              
            return base.ExecCommandOnNode(cmdGroup, cmd, nCmdexecopt, pvaIn, pvaOut);
        }

        private void ExecNodeCommand(uint commandID)
        {
            switch(commandID)
            {
                case Commands.LevelsNodeLink:
                    LinkNode();
                    break;

                default:
                    throw new ComSpecificException((int) Constants.OLECMDERR_E_NOTSUPPORTED);
            }
        }

        private void LinkNode()
        {
            var ofd = new OpenFileDialog
                          {
                              Filter = string.Format("{0} (*.{1})|*.{1}", Resources.LevelDatabaseFile, DbFileExtension),
                              Title = Resources.OpenLevelDatabaseFile,
                          };
            if (ofd.ShowDialog() != DialogResult.OK)
                return;

            var relativePath = PathHelper.GetRelativePath(ProjectMgr.ProjectFolder, ofd.FileName);
            ChangePath(relativePath);
        }

        private void ChangePath(string relativePath)
        {
            RemoveAllChildren();
            _path = relativePath;
            LoadChildren();
        }

        public override void ReloadItem()
        {
            RemoveAllChildren();
            LoadChildren();
        }

        internal void LoadChildren()
        {
            string path;
            if (string.IsNullOrEmpty(_path))
                path = PathHelper.GetFullPath(ProjectMgr.ProjectFolder, "Export", "DB", "game.db3");
            else
            {
                path = _path;
                if (!Path.IsPathRooted(path))
                    path = PathHelper.GetFullPath(ProjectMgr.ProjectFolder, path);
            }

            if (!File.Exists(path))
            {
                MessageBox.Show(string.Format(Resources.FileNotFoundFormatString, path), Resources.FileNotFound, MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            var engine = ((CideProjectNode) ProjectMgr).Engine;
            engine.Init(IntPtr.Zero, ProjectMgr.ProjectFolder);
        }
    }
}
