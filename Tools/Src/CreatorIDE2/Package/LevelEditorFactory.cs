using System;
using System.Runtime.InteropServices;
using Microsoft.VisualStudio.Project;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class LevelEditorFactory: CideEditorFactory<LevelNode>
    {
        private const string GuidString = "FF22E6FD-31DB-485B-AC36-D51DD9D7C11D";

        public LevelEditorFactory(CidePackage package):
            base(package)
        {
        }

        public override EditorInstanceDescriptor CreateEditorInstance(VsCreateEditorFlags flags, string mkDocument, string physicalView, LevelNode hierarchy, IntPtr punkDocDataExisting)
        {
            var pane = new LevelEditorPane(ServiceProvider, hierarchy);
            var result = new EditorInstanceDescriptor
                             {
                                 DocData = pane,
                                 DocView = pane,
                                 Flags = VsEditorCreateDocWinFlags.CreateNewWindow | VsEditorCreateDocWinFlags.Dockable
                             };
            return result;
        }

        protected override void Close()
        {
        }

        protected override string MapLogicalView(Guid logicalViewID)
        {
            return string.Empty;
        }
    }
}
