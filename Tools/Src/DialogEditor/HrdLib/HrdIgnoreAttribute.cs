using System;

namespace HrdLib
{
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    public class HrdIgnoreAttribute: Attribute
    {
        private readonly bool _ignore;

        public bool Ignore{get { return _ignore; }}

        public HrdIgnoreAttribute(bool ignore)
        {
            _ignore = ignore;
        }

        public HrdIgnoreAttribute() :
            this(true)
        {
        }
    }
}
