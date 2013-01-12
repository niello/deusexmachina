namespace CreatorIDE.Engine
{
    public interface IAttrEditorProvider
    {
        object GetEditor(AttrProperty property);
    }
}
