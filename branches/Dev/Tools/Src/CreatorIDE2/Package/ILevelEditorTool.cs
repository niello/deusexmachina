using System;
using Microsoft.VisualStudio.Shell;

namespace CreatorIDE.Package
{
    /// <summary>
    /// Basic interface for tool windows associated with an editor
    /// </summary>
    public interface ILevelEditorTool
    {
        event EventHandler SelectionChanged;
        
        ILevelEditorToolPane Pane { get; }
        SelectionContainer Selection { get; }

        void Initialize(LevelNode level);
    }
}
