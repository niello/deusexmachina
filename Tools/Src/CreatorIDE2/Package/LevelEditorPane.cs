using System;
using System.Collections.Generic;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Drawing;
using System.Windows.Forms;
using CreatorIDE.Engine;
using CreatorIDE.Core;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.OLE.Interop;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.TextManager.Interop;
using IServiceProvider = System.IServiceProvider;

namespace CreatorIDE.Package
{
    public class LevelEditorPane : WindowPane,
                                IVsPersistDocData,  //to Enable persistence functionality for document data
                                IPersistFileFormat, //to enable the programmatic loading or saving of an object 
        //in a format specified by the user.
                                IVsFileChangeEvents,//to notify the client when file changes on disk
                                IVsDocDataFileChangeControl, //to Determine whether changes to files made outside 
        //of the editor should be ignored
                                IVsFileBackup,      //to support backup of files. Visual Studio File Recovery 
        //backs up all objects in the Running Document Table that 
        //support IVsFileBackup and have unsaved changes.
                                IVsStatusbarUser   //support updating the status bar
    {
        private readonly List<CommandID> _editorCommands = new List<CommandID>();
        private readonly Control _control;
        private readonly EngineHostControl _engineHostControl;
        private readonly LevelNode _levelNode;

        public LevelNode LevelNode{get { return _levelNode; }}

        public LevelEditorPane(IServiceProvider provider, LevelNode levelNode):
            base(provider)
        {
            if (levelNode == null)
                throw new ArgumentNullException("levelNode");

            _control = new Control {BackColor = Color.CornflowerBlue, Dock = DockStyle.Fill};
            _engineHostControl = new EngineHostControl {Width = 800, Height = 600, Location = new Point(3, 3), Parent = _control};
            _engineHostControl.Load += OnEngineHostControlLoad;
            _levelNode = levelNode;
        }

        private void OnEngineHostControlLoad(object sender, EventArgs e)
        {
            var engine = _levelNode.ProjectMgr.Engine;
            engine.PathRequest += OnEnginePathRequest;
            engine.MouseClick += OnEngineMouseClick;
            engine.Init(_engineHostControl.Handle, @"..\..\..\..\InsanePoet\Bin\data");
            if (engine.GetLevelCount() > 0)
            {
                var level = engine.GetLevelRecord(0);
                engine.LoadLevel(level.ID);
            }
            _engineHostControl.Engine = engine;

            LoadEditorTools();
        }

        private void LoadEditorTools()
        {
            var pkgType = _levelNode.ProjectMgr.Package.GetType();
            var attrs = pkgType.GetCustomAttributes(typeof (ProvideToolWindowAttribute), true);
            foreach(ProvideToolWindowAttribute attr in attrs)
            {
                if (attr.ToolType == null || !typeof(ILevelEditorToolPane).IsAssignableFrom(attr.ToolType))
                    continue;

                var pane = _levelNode.ProjectMgr.Package.FindToolWindow(attr.ToolType, 0, true);
                Debug.Assert(pane != null);
                var tool = ((ILevelEditorToolPane) pane).GetTool();
                tool.Initialize(_levelNode);

                tool.SelectionChanged += OnToolWindowSelectionChanged;
            }
        }

        private static void OnToolWindowSelectionChanged(object sender, EventArgs e)
        {
            var tool = sender as ILevelEditorTool;
            if (tool == null || tool.Pane == null)
                return;

            var trackSelection = tool.Pane.TrackSelection;
            if (trackSelection != null)
                trackSelection.OnSelectChange(tool.Selection);
        }

        private void OnEngineMouseClick(object sender, EngineMouseClickEventArgs e)
        {
            // TODO: Process mouse click
        }

        private void OnEnginePathRequest(object sender, EnginePathRequestEventArgs e)
        {
            if (e.Handled)
                return;

            string path;
            if (!_levelNode.TryResolvePath(e.Path, out path))
                return;

            e.NormalizedPath = path;
            e.Handled = true;
        }

        #region Overrides of WindowPane

        public override IWin32Window Window
        {
            get { return _control; }
        }

        protected override void Initialize()
        {
            base.Initialize();

            var menuGlobalService = _levelNode.ProjectMgr.Package.GetService<IMenuCommandService>();
            AddCommand(menuGlobalService, Commands.LevelToolbarObjectBrowser, OnObjectBrowserCommand);
        }

        private void AddCommand(IMenuCommandService commandService, int levelToolbarCommandID, EventHandler handler)
        {
            if (commandService == null)
                throw new ArgumentNullException("commandService");

            var cmdID = new CommandID(Commands.LevelToolbarGuid, levelToolbarCommandID);
            var cmd = new OleMenuCommand(handler, cmdID);
            cmd.BeforeQueryStatus += OnBeforeQueryCommandStatus;
            commandService.AddCommand(cmd);
            _editorCommands.Add(cmdID);
        }

        private void OnBeforeQueryCommandStatus(object sender, EventArgs e)
        {
            var command = sender as OleMenuCommand;
            if (command != null)
                command.Enabled = _levelNode.ProjectMgr.Package.IsActive(Commands.LevelEditorCmdGuid) == true;
        }

        private void OnObjectBrowserCommand(object sender, EventArgs e)
        {
            var pkg = _levelNode.ProjectMgr.Package;
            var wnd = pkg.FindToolWindow(typeof (LevelObjectBrowserPane), 0, false);
            var frame = wnd == null ? null : wnd.Frame as IVsWindowFrame;
            if(frame!=null)
                ErrorHandler.ThrowOnFailure(frame.Show());
        }

        protected override void OnClose()
        {
            base.OnClose();

            var cmdService = _levelNode.ProjectMgr.Package.GetService<IMenuCommandService>();
            Debug.Assert(cmdService != null);

            foreach(var cmdID in _editorCommands)
            {
                var cmd = cmdService.FindCommand(cmdID);
                if (cmd != null)
                    cmdService.RemoveCommand(cmd);
            }
        }

        #endregion

        #region Implementation of IVsPersistDocData

        public bool IsReloadable { get { return false; } }

        int IVsPersistDocData.GetGuidEditorType(out Guid pClassID)
        {
            pClassID = typeof (LevelEditorFactory).GUID;
            return HResult.Ok;
        }

        int IVsPersistDocData.IsDocDataDirty(out int pfDirty)
        {
            return ((IPersistFileFormat) this).IsDirty(out pfDirty);
        }

        int IVsPersistDocData.SetUntitledDocPath(string pszDocDataPath)
        {
            return HResult.False;
        }

        int IVsPersistDocData.LoadDocData(string pszMkDocument)
        {
            return HResult.False;
        }

        int IVsPersistDocData.SaveDocData(VSSAVEFLAGS dwSave, out string pbstrMkDocumentNew, out int pfSaveCanceled)
        {
            throw new NotImplementedException();
        }

        int IVsPersistDocData.Close()
        {
            return HResult.False;
        }

        int IVsPersistDocData.OnRegisterDocData(uint docCookie, IVsHierarchy pHierNew, uint itemidNew)
        {
            return HResult.False;
        }

        int IVsPersistDocData.RenameDocData(uint grfAttribs, IVsHierarchy pHierNew, uint itemidNew, string pszMkDocumentNew)
        {
            return HResult.False;
        }

        int IVsPersistDocData.IsDocDataReloadable(out int pfReloadable)
        {
            return ComHelper.WrapFunction(false, () => IsReloadable, out pfReloadable,
                                          isReloadable => IsReloadable ? 1 : 0);
        }

        int IVsPersistDocData.ReloadDocData(uint grfFlags)
        {
            return HResult.False;
        }

        #endregion

        #region Implementation of IPersist

        public Guid ClassID { get { return GetType().GUID; } }
        public bool IsDirty { get; private set; }

        int IPersist.GetClassID(out Guid pClassID)
        {
            return ComHelper.WrapFunction(false, () => ClassID, out pClassID);
        }

        int IPersistFileFormat.IsDirty(out int pfIsDirty)
        {
            return ComHelper.WrapFunction(false, () => IsDirty, out pfIsDirty, isDirty => isDirty ? 1 : 0);
        }

        int IPersistFileFormat.InitNew(uint nFormatIndex)
        {
            return HResult.False;
        }

        int IPersistFileFormat.Load(string pszFilename, uint grfMode, int fReadOnly)
        {
            return HResult.False;
        }

        int IPersistFileFormat.Save(string pszFilename, int fRemember, uint nFormatIndex)
        {
            return HResult.False;
        }

        int IPersistFileFormat.SaveCompleted(string pszFilename)
        {
            return HResult.False;
        }

        int IPersistFileFormat.GetCurFile(out string ppszFilename, out uint pnFormatIndex)
        {
            throw new NotImplementedException();
        }

        int IPersistFileFormat.GetFormatList(out string ppszFormatList)
        {
            return ComHelper.WrapFunction(false, GetFormatList, out ppszFormatList);
        }

        private static string GetFormatList()
        {
            return "*.*|*.*";
        }

        #endregion

        #region Implementation of IPersistFileFormat

        int IPersistFileFormat.GetClassID(out Guid pClassID)
        {
            return ((IPersist)this).GetClassID(out pClassID);
        }

        #endregion

        #region Implementation of IVsFileChangeEvents

        int IVsFileChangeEvents.FilesChanged(uint cChanges, string[] rgpszFile, uint[] rggrfChange)
        {
            return HResult.False;
        }

        int IVsFileChangeEvents.DirectoryChanged(string pszDirectory)
        {
            return HResult.False;
        }

        #endregion

        #region Implementation of IVsDocDataFileChangeControl

        int IVsDocDataFileChangeControl.IgnoreFileChanges(int fIgnore)
        {
            return HResult.False;
        }

        #endregion

        #region Implementation of IVsFileBackup

        int IVsFileBackup.BackupFile(string pszBackupFileName)
        {
            return HResult.False;
        }

        int IVsFileBackup.IsBackupFileObsolete(out int pbObsolete)
        {
            throw new NotImplementedException();
        }

        #endregion

        #region Implementation of IVsStatusbarUser

        int IVsStatusbarUser.SetInfo()
        {
            return HResult.False;
        }

        #endregion
    }
}
