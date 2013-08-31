using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using Microsoft.VisualStudio.Project;
using OleConstants = Microsoft.VisualStudio.OLE.Interop.Constants;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class LevelNode: CideFileNode
    {
        private const string GuidString = "0CEA25A5-4799-4461-A7FB-79C77EA04743";
        public const string FileExtension = "db3";

        public override Guid ItemTypeGuid
        {
            get { return typeof(LevelNode).GUID; }
        }

        public override int ImageIndex
        {
            get { return ProjectMgr.ImageListOffset + Images.Map; }
        }

        protected override Guid DefaultEditorTypeID { get { return typeof(LevelEditorFactory).GUID; } }

        public LevelNode(CideProjectNode projectNode, ProjectElement element):
            base(projectNode,element)
        {}

        protected override bool DisableCmdInCurrentMode(Guid commandGroup, uint command)
        {
            if (commandGroup == Commands.LevelsNodeMenuGuid)
                return false;
            return base.DisableCmdInCurrentMode(commandGroup, command);
        }

        protected override bool ExecCommandOnNode(Guid cmdGroup, uint cmd, uint nCmdexecopt, IntPtr pvaIn, IntPtr pvaOut)
        {
            if (cmdGroup == Commands.LevelsNodeMenuGuid)
            {
                ExecNodeCommand(cmd);
                return true;
            }

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
                    throw new ComSpecificException((int) OleConstants.OLECMDERR_E_NOTSUPPORTED);
            }
        }

        private void LinkNode()
        {
            var ofd = new OpenFileDialog
                          {
                              Filter = string.Format("{0} (*.{1})|*.{1}", Resources.LevelDatabaseFile, FileExtension),
                              Title = Resources.OpenLevelDatabaseFile,
                          };
            if (ofd.ShowDialog() != DialogResult.OK)
                return;

            var relativePath = PathHelper.GetRelativePath(ProjectMgr.ProjectFolder, ofd.FileName);
        }

        public bool TryResolvePath(string path, out string fullPath)
        {
            if(path==null)
            {
                fullPath = null;
                return false;
            }

            int idx = path.IndexOf(CidePathHelper.ScopeSeparatorChar);
            if(idx<=0)
            {
                fullPath = null;
                return false;
            }

            var key = path.Substring(0, idx).Trim().ToLowerInvariant();
            if(key.Length<=1)
            {
                fullPath = null;
                return false;
            }

            switch(key)
            {
                case Configuration.HomeScope:
                    fullPath = ProjectMgr.GetProjectProperty(CideProjectElements.HomeFolder);
                    if (fullPath == null)
                        return false;
                    break;

                case Configuration.ProjectScope:
                    fullPath = Path.GetDirectoryName(ProjectMgr.Url);
                    break;

                default:
                    CideFolderNode folderNode;
                    if(!ProjectMgr.TryGetFromScopeMap(key, out folderNode))
                    {
                        fullPath = null;
                        return false;
                    }

                    fullPath = folderNode.GetMkDocument();
                    break;        
            }

            Debug.Assert(fullPath != null);
            if (idx < path.Length - 1)
                fullPath = Path.Combine(fullPath, path.Substring(idx + 1));
            if (!Path.IsPathRooted(fullPath) && !CidePathHelper.IsInScope(fullPath))
            {
                var projectDir = Path.GetDirectoryName(ProjectMgr.GetMkDocument());
                Debug.Assert(projectDir != null);
                fullPath = Path.Combine(projectDir, fullPath);
            }
            fullPath = Path.GetFullPath(fullPath);
            return true;
        }
    }
}
