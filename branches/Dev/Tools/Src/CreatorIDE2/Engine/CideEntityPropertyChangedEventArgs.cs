using System.ComponentModel;

namespace CreatorIDE.Engine
{
    public class CideEntityPropertyChangedEventArgs : PropertyChangedEventArgs
    {
        private readonly CideEntity _entity;
        private readonly object _oldValue, _newValue;

        public CideEntity Entity { get { return _entity; } }

        public object OldValue { get { return _oldValue; } }

        public object NewValue{get { return _newValue; }}

        public CideEntityPropertyChangedEventArgs(CideEntity entity, string propertyName, object newValue, object oldValue) :
            base(propertyName)
        {
            _entity = entity;
            _oldValue = oldValue;
            _newValue = newValue;
        }
    }
}
