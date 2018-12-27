using System;

namespace HrdLib
{
    public class HrdAttribute:HrdElement
    {
        public string Value { get; set; }

        public bool SerializeAsQuotedString { get; set; }

        public HrdAttribute(string name) :
            base(name)
        {
            SerializeAsQuotedString = true;
        }

        public HrdAttribute(string name, string value) :
            this(name)
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
