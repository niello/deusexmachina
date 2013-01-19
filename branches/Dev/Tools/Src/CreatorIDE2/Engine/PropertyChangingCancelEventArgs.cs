using System.ComponentModel;

namespace CreatorIDE.Engine
{
    public class PropertyChangingCancelEventArgs<T>: PropertyChangingEventArgs
    {
        private readonly T _oldValue;

        public T OldValue { get { return _oldValue; } }
        public T NewValue { get; set; }

        public bool Cancel { get; set; }

        public PropertyChangingCancelEventArgs(string propertyName, T oldValue, T newValue)
            : base(propertyName)
        {
            NewValue = newValue;
            _oldValue = oldValue;
        }
    }
}
