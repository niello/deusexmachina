using System;
using System.Reflection;

namespace HrdLib
{
    [AttributeUsage(AttributeTargets.Parameter)]
    public sealed class HrdPropertySetterAttribute : Attribute
    {
        private readonly string _propertyName;

        public string PropertyName
        {
            get { return _propertyName; }
        }

        internal ParameterInfo Parameter { get; set; }

        public HrdPropertySetterAttribute(string propertyName)
        {
            _propertyName = propertyName;
        }
    }
}
