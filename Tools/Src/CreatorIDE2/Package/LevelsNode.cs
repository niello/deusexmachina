using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using Microsoft.VisualStudio.Project;
using Constants = Microsoft.VisualStudio.OLE.Interop.Constants;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class LevelsNode: FileNode
    {
        private const string GuidString = "0CEA25A5-4799-4461-A7FB-79C77EA04743";
        private const string DbFileExtension = "db3";

        public LevelsNode(CideProjectNode projectNode, ProjectElement element):
            base(projectNode,element)
        {}

        public override Guid ItemTypeGuid
        {
            get { return typeof (LevelsNode).GUID; }
        }

        public override int ImageIndex
        {
            get { return ((CideProjectNode) ProjectMgr).ImageListOffset + Images.Globe; }
        }

        protected override bool DisableCmdInCurrentMode(Guid commandGroup, uint command)
        {
            if (commandGroup == Commands.LevelsNodeMenuGuid)
                return false;
            return base.DisableCmdInCurrentMode(commandGroup, command);
        }

        protected override void ExecCommandOnNode(Guid cmdGroup, uint cmd, uint nCmdexecopt, IntPtr pvaIn, IntPtr pvaOut)
        {
            if (cmdGroup == Commands.LevelsNodeMenuGuid)
            {
                ExecNodeCommand(cmd);
                return;
            }

            base.ExecCommandOnNode(cmdGroup, cmd, nCmdexecopt, pvaIn, pvaOut);
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
        }
    }
}
