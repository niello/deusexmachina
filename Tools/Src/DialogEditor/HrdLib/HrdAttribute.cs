using System;

namespace HrdLib
{
    public class HrdAttribute:HrdElement
    {
        public object Value { get; set; }

        public HrdAttribute(string name):
            base(name)
        {}

        public HrdAttribute(string name,object value):
            base(name)
        {
            Value = value;
        }

        public HrdAttribute():
            base(null)
        {}

        public override void AddElement(HrdElement elementBase)
        {
            throw new NotSupportedException("Attribute can't contains any other element.");
        }
    }
}
