using System;
using System.Reflection;

namespace HrdLib
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    public sealed class HrdSerializationAttribute : HrdIgnoreAttribute, ICloneable
    {
        public int Order { get; set; }

        internal PropertyInfo PropertyInfo { get; set; }

        public HrdSerializationAttribute(bool ignore) :
            base(ignore)
        {
            Order = int.MaxValue;
        }

        public HrdSerializationAttribute() :
            this(false)
        {
        }

        public HrdSerializationAttribute Clone()
        {
            return new HrdSerializationAttribute(Ignore) {Order = Order, PropertyInfo = PropertyInfo};
        }

        object ICloneable.Clone()
        {
            return Clone();
        }
    }
}
