using System;
using System.Linq;
using System.Collections.Generic;

namespace HrdLib
{
    public class HrdElement
    {
        private readonly List<HrdElement> _elements = new List<HrdElement>();

        protected List<HrdElement> Elements
        {
            get { return _elements; }
        }

        public string Name { get; set; }

        public int ChildrenCount
        {
            get { return _elements.Count; }
        }

        protected HrdElement(string name)
        {
            Name = name;
        }

        public virtual void AddElement(HrdElement elementBase)
        {
            if (elementBase == null)
                throw new ArgumentNullException("elementBase");

            if (ReferenceEquals(elementBase, this))
                throw new Exception("Element can't conatains itself.");

            if (elementBase.Name != null && _elements.Any(el => string.Equals(el.Name, elementBase.Name, StringComparison.Ordinal)))
                throw new Exception(string.Format("Element with name '{0}' is already added.", elementBase.Name));

            _elements.Add(elementBase);
        }

        public void AddElements(params HrdElement[] elements)
        {
            AddElements((IEnumerable<HrdElement>) elements);
        }

        public void AddElements(IEnumerable<HrdElement> elements)
        {
            foreach(var el in elements)
                AddElement(el);
        }

        public TElement GetElement<TElement>(string name)
            where TElement:HrdElement
        {
            if (name == null)
                throw new ArgumentNullException("name");

            return
                _elements.OfType<TElement>().FirstOrDefault(el => string.Equals(el.Name, name, StringComparison.Ordinal));
        }

        public IEnumerable<TElement> GetUnnamedElements<TElement>()
            where TElement:HrdElement
        {
            return _elements.OfType<TElement>().Where(el => el.Name == null);
        }

        public IEnumerable<HrdElement> GetElements()
        {
            return _elements.AsEnumerable();
        }

        public IEnumerable<HrdElement> GetUnnamedElemetns()
        {
            return _elements.Where(el => el.Name == null);
        }

        public IEnumerable<TElement> GetElements<TElement>()
            where TElement:HrdElement
        {
            return _elements.OfType<TElement>();
        }

        public HrdElement GetElementAt(int index)
        {
            return _elements[index];
        }
    }
}
