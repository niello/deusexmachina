using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Shell.Interop;
using IServiceProvider = Microsoft.VisualStudio.OLE.Interop.IServiceProvider;

namespace CreatorIDE.Package
{
    public abstract class CideEditorFactory<TNode>: IVsEditorFactory
        where TNode:HierarchyNode
    {
        private readonly CidePackage _package;

        public ServiceProvider ServiceProvider { get; private set; }

        public CidePackage Package { get { return _package; } }

        protected CideEditorFactory(CidePackage package)
        {
            if (package == null)
                throw new ArgumentNullException("package");

            _package = package;
        }

        int IVsEditorFactory.CreateEditorInstance(uint grfCreateDoc, string pszMkDocument, string pszPhysicalView, IVsHierarchy pvHier, uint itemid, IntPtr punkDocDataExisting, out IntPtr ppunkDocView, out IntPtr ppunkDocData, out string pbstrEditorCaption, out Guid pguidCmdUI, out int pgrfCDW)
        {
            EditorInstanceDescriptor descr;
            var result = ComHelper.WrapFunction(false, CreateEditorInstanceInternal, (VsCreateEditorFlags)grfCreateDoc, pszMkDocument,
                                                pszPhysicalView, pvHier, (VsItemID)itemid, punkDocDataExisting, out descr);
            descr = descr ?? new EditorInstanceDescriptor();
            ppunkDocData = descr.DocDataPtr;
            ppunkDocView = descr.DocViewPtr;
            pbstrEditorCaption = descr.EditorCaption;
            pguidCmdUI = descr.CmdID;
            pgrfCDW = (int) descr.Flags;
            return result;
        }

        private EditorInstanceDescriptor CreateEditorInstanceInternal(VsCreateEditorFlags flags, string mkDocument, string physicalView, IVsHierarchy hierarchy, VsItemID itemID, IntPtr punkDocDataExisting)
        {
            TNode node;
            if (hierarchy != null)
            {
                Guid projectIDGuid;
                hierarchy.GetGuidProperty(VsItemID.Root, (int)VsHPropID.ProjectIDGuid, out projectIDGuid);
                // Trying to find a node in the package
                if (!_package.TryGetNode(projectIDGuid, itemID, out node))
                    node = null;
            }
            else node = null;

            Debug.Assert(node != null);
            return CreateEditorInstance(flags, mkDocument, physicalView, node, punkDocDataExisting);
        }

        public abstract EditorInstanceDescriptor CreateEditorInstance(VsCreateEditorFlags flags, string mkDocument,
                                                                      string physicalView, TNode node,
                                                                      IntPtr punkDocDataExisting);
        

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
        public VsEditorCreateDocWinFlags Flags { get; set; }

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
