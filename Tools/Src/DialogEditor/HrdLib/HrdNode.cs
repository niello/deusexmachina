using System;

namespace HrdLib
{
    public class HrdNode:HrdElement
    {
        public HrdNode(string name):
            base(name)
        {}

        public HrdNode():
            base(null)
        {}

        public override void AddElement(HrdElement elementBase)
        {
            if (elementBase.Name == null)
                throw new Exception("An element without the name can't be added to HrdNode.");

            base.AddElement(elementBase);
        }
    }
}
