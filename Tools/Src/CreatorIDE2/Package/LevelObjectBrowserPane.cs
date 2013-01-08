using System;
using System.Runtime.InteropServices;
using System.Windows.Forms;
using Microsoft.VisualStudio.Shell;

namespace CreatorIDE.Package
{
    [ComVisible(true), Guid(GuidString)]
    public class LevelObjectBrowserPane : ToolWindowPane, ILevelEditorToolPane
    {
        private const string GuidString = "EEAF75AC-A373-4979-84F3-5A4C1228DFBA";

        private readonly LevelObjectBrowserControl _objectBrowser;

        public LevelObjectBrowserPane() :
            base(null)
        {
            Caption = SR.GetString(SR.LevelObjectBrowserCaption);
            BitmapResourceID = Images.ImageStripResID;
            BitmapIndex = Images.GlobeView;

            _objectBrowser = new LevelObjectBrowserControl();
        }

        public override IWin32Window Window
        {
            get { return _objectBrowser; }
        }

        public ILevelEditorTool GetTool()
        {
            return _objectBrowser;
        }
    }
}
