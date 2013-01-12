using Microsoft.VisualStudio.Shell.Interop;

namespace CreatorIDE.Package
{
    public interface ILevelEditorToolPane
    {
        ILevelEditorTool GetTool();
        ITrackSelection TrackSelection { get; }
    }
}
