using System;
using System.Reflection;

namespace HrdLib
{
    internal class HrdSerializerPropertyInfo
    {
        private readonly PropertyInfo _propertyInfo;
        private readonly string _name;

        public PropertyInfo PropertyInfo
        {
            get { return _propertyInfo; }
        }

        public string Name
        {
            get { return _name ?? PropertyInfo.Name; }
        }

        public bool HasGetMethod
        {
            get { return PropertyInfo.GetGetMethod(false) != null; }
        }

        public bool HasSetMethod
        {
            get { return PropertyInfo.GetSetMethod(false) != null; }
        }

        public int ConstructorArgumentIndex { get; set; }

        public HrdSerializerPropertyInfo(PropertyInfo propertyInfo) :
            this(propertyInfo, null)
        {
        }

        public HrdSerializerPropertyInfo(PropertyInfo propertyInfo, string customName)
        {
            if (propertyInfo == null)
                throw new ArgumentNullException("propertyInfo");

            _propertyInfo = propertyInfo;
            _name = customName;
            ConstructorArgumentIndex = -1;
        }
    }
}
