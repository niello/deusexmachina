namespace CreatorIDE.Package
{
    /// <summary>
    /// Basic interface for tool windows associated with an editor
    /// </summary>
    public interface ILevelEditorTool
    {
        void Initialize(LevelNode level);
    }
}
