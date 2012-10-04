using System;

namespace HrdLib
{
    public class HrdArray:HrdElement
    {
        public HrdArray(string name):
            base(name)
        {}

        public HrdArray():
            base(null)
        {}

        public override void AddElement(HrdElement elementBase)
        {
            if(elementBase.Name!=null)
                throw new Exception("Array can't contains named elements.");

            base.AddElement(elementBase);
        }

        public int Count { get { return _elements.Count;}}

        public HrdElement this[int index] { get { return _elements[index]; } }
    }
}
