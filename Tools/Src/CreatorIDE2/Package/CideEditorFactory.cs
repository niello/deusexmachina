using System;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using IServiceProvider = Microsoft.VisualStudio.OLE.Interop.IServiceProvider;

namespace CreatorIDE.Package
{
    public abstract class CideEditorFactory: IVsEditorFactory
    {
        private readonly CidePackage _package;

        public ServiceProvider ServiceProvider { get; private set; }

        public CidePackage Package { get { return _package; } }

        protected CideEditorFactory(CidePackage package)
        {
            _package = package;
        }

        int IVsEditorFactory.CreateEditorInstance(uint grfCreateDoc, string pszMkDocument, string pszPhysicalView, IVsHierarchy pvHier, uint itemid, IntPtr punkDocDataExisting, out IntPtr ppunkDocView, out IntPtr ppunkDocData, out string pbstrEditorCaption, out Guid pguidCmdUI, out int pgrfCDW)
        {
            EditorInstanceDescriptor descr;
            var result = ComHelper.WrapFunction(false, CreateEditorInstance, grfCreateDoc, pszMkDocument,
                                                pszPhysicalView, pvHier, (VsItemID)itemid, punkDocDataExisting, out descr);
            descr = descr ?? new EditorInstanceDescriptor();
            ppunkDocData = descr.DocDataPtr;
            ppunkDocView = descr.DocViewPtr;
            pbstrEditorCaption = descr.EditorCaption;
            pguidCmdUI = descr.CmdID;
            pgrfCDW = descr.PgrfCdw;
            return result;
        }

        public abstract EditorInstanceDescriptor CreateEditorInstance(uint createDoc, string mkDocument,
                                                                      string physicalView, IVsHierarchy hierarchy,
                                                                      VsItemID itemID, IntPtr punkDocDataExisting);
        

        int IVsEditorFactory.SetSite(IServiceProvider psp)
        {
            return ComHelper.WrapAction(false, s => ServiceProvider = s == null ? null : new ServiceProvider(s), psp);
        }

        int IVsEditorFactory.Close()
        {
            return ComHelper.WrapAction(false, Close);
        }

        protected abstract void Close();

        int IVsEditorFactory.MapLogicalView(ref Guid rguidLogicalView, out string pbstrPhysicalView)
        {
            return ComHelper.WrapFunction(false, MapLogicalView, rguidLogicalView, out pbstrPhysicalView);
        }

        protected abstract string MapLogicalView(Guid logicalViewID);
    }

    public class EditorInstanceDescriptor
    {
        private object _docView, _docData;

        internal IntPtr DocViewPtr { get; private set; }
        internal IntPtr DocDataPtr { get; private set; }

        public string EditorCaption { get; set; }
        public Guid CmdID { get; set;}
        public int PgrfCdw { get; set; }

        public object DocView
        {
            get { return _docView; }
            set
            {
                _docView = value;
                DocViewPtr = _docView == null ? IntPtr.Zero : Marshal.GetIUnknownForObject(_docView);
            }
        }

        public object DocData
        {
            get { return _docData; }
            set
            {
                _docData = value;
                DocDataPtr = _docData == null ? IntPtr.Zero : Marshal.GetIUnknownForObject(_docData);
            }
        }
    }
}
