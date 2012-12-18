using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;
using CreatorIDE.Engine;
using EnvDTE;
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
                                IVsStatusbarUser,   //support updating the status bar
                                IExtensibleObject  //so we can get the automation object
    {
        private readonly Control _control;
        private readonly LevelNode _levelNode;

        public LevelEditorPane(IServiceProvider provider, LevelNode levelNode):
            base(provider)
        {
            if (levelNode == null)
                throw new ArgumentNullException("levelNode");

            _control = new Control();
            _control.SizeChanged += OnControlSizeChanged;
            _levelNode = levelNode;
            var engine = _levelNode.ProjectMgr.Engine;
            engine.PathRequest += OnEnginePathRequest;
        }

        private void OnControlSizeChanged(object sender, EventArgs e)
        {
            var control = (Control) sender;
            Debug.Assert(ReferenceEquals(control, _control));

            var engine = _levelNode.ProjectMgr.Engine;
            if (control.Width == 0 || control.Height == 0 || engine.IsInitialized)
                return;

            string path;
            var homePath = CidePathHelper.AddScope(Configuration.HomeScope, string.Empty);
            if (!_levelNode.TryResolvePath(homePath, out path))
                path = Configuration.AppFolder;

            engine.Init(control.Handle, path);
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

        #region Implementation of IExtensibleObject

        void IExtensibleObject.GetAutomationObject(string Name, IExtensibleObjectSite pParent, out object ppDisp)
        {
            throw new NotImplementedException();
        }

        #endregion
    }
}
