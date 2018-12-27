using System;
using System.ComponentModel;

namespace DialogLogic
{
    public class DialogCharacterPropertyDescriptor:PropertyDescriptor
    {
        private readonly Type _propertyType;

        internal event EventHandler<IsReadOnlyRequestEArgs> IsReadOnlyRequest;

        public object DefaultValue { get; private set; }
        public bool HasDefaultValue { get; private set; }

        public DialogCharacterPropertyDescriptor(string name, Type propertyType, Attribute[] attrs)
            : base(name, attrs)
        {
            _propertyType = propertyType;
            HasDefaultValue = false;
        }

        public DialogCharacterPropertyDescriptor(string name, Type propertyType, object defaultValue, Attribute[] attrs):
            base(name,attrs)
        {
            _propertyType = propertyType;
            HasDefaultValue = true;
            DefaultValue = defaultValue;
        }

        private bool GetIsReadOnly()
        {
            if(IsReadOnlyRequest!=null)
            {
                var args = new IsReadOnlyRequestEArgs();
                IsReadOnlyRequest(this, args);
                return args.HandledBy != null && args.IsReadOnly;
            }

            return false;
        }

        public override bool CanResetValue(object component)
        {
            return HasDefaultValue;
        }

        public override object GetValue(object component)
        {
            return ((DialogCharacter) component).GetValue(_propertyType, Name,
                                                          HasDefaultValue ? DefaultValue : null);
        }

        public override void ResetValue(object component)
        {
            if (HasDefaultValue)
                ((DialogCharacter) component).SetValue(Name, DefaultValue);
        }

        public override void SetValue(object component, object value)
        {
            ((DialogCharacter) component).SetValue(Name, value);
        }

        public override bool ShouldSerializeValue(object component)
        {
            return true;
        }

        public override Type ComponentType
        {
            get { return typeof (DialogCharacter); }
        }

        public override bool IsReadOnly
        {
            get { return Name == "Name" || Name == "Id" || GetIsReadOnly(); }
        }

        public override Type PropertyType
        {
            get { return _propertyType; }
        }
    }

    internal class IsReadOnlyRequestEArgs:EventArgs
    {
        public bool IsReadOnly { get; set; }
        public ICharacterContainer HandledBy { get; set; }
    }
}
