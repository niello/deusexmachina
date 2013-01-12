using System;
using System.ComponentModel;

namespace CreatorIDE.Engine
{
    internal class AttrPropertyDescriptor : PropertyDescriptor
    {
        private readonly AttrProperty _prop;

        public AttrPropertyDescriptor(AttrProperty prop, Attribute[] attrs)
            : base(prop.Name, attrs)
        {
            _prop = prop;
        }

        public override bool CanResetValue(object component)
        {
            return false;
        }

        public override Type ComponentType
        {
            get { return typeof (CideEntity); }
        }

        public override object GetValue(object component)
        {
            return _prop.Value;
        }

        public override string Description
        {
            get { return _prop.Description; }
        }

        public override string Category
        {
            get { return _prop.Category; }
        }

        public override string DisplayName
        {
            get { return _prop.Name; }
        }

        public override bool IsReadOnly
        {
            get { return _prop.ReadOnly; }
        }

        public override void ResetValue(object component)
        {
            //Have to implement
            //???use default values?
        }

        public override bool ShouldSerializeValue(object component)
        {
            return false;
        }

        public override void SetValue(object component, object value)
        {
            _prop.Value = value;
        }

        public override Type PropertyType
        {
            get { return _prop.Type; }
        }

        public override object GetEditor(Type editorBaseType)
        {
            //if (_prop.IsResourceProp) return new ResourceStringEditor(_prop.ResourceDir, _prop.ResourceExt);
            object editor;
            if (_prop.EditorProvider != null && (editor = _prop.EditorProvider.GetEditor(_prop)) != null)
                return editor;
            return base.GetEditor(editorBaseType);
        }
    }
}
