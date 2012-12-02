using System;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;
using Microsoft.VisualStudio.Shell.Interop;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class LevelEditorFactory: CideEditorFactory
    {
        private const string GuidString = "FF22E6FD-31DB-485B-AC36-D51DD9D7C11D";

        public LevelEditorFactory(CidePackage package):
            base(package)
        {
        }

        public override EditorInstanceDescriptor CreateEditorInstance(uint createDoc, string mkDocument, string physicalView, IVsHierarchy hierarchy, VsItemID itemID, IntPtr punkDocDataExisting)
        {
            var pane = new LevelEditorPane(ServiceProvider);
            var result = new EditorInstanceDescriptor
                             {
                                 DocData = pane,
                                 DocView = pane,
                                 EditorCaption = "EC"
                             };
            return result;
        }

        protected override void Close()
        {
            throw new NotImplementedException();
        }

        protected override string MapLogicalView(Guid logicalViewID)
        {
            return string.Empty;
        }
    }
}
