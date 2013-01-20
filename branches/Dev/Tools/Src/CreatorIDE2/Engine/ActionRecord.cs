namespace CreatorIDE.Engine
{
    public class ActionRecord
    {
        public object OldValue { get; internal set; }
        public object NewValue { get; internal set; }
        public string UID { get; internal set; }
        public bool IsCategory { get; internal set; }
    }
}
