namespace HrdLib
{
    public class HrdNode:HrdElement
    {
        /// <summary>
        /// Tells the serializer that this node is not a part of the tree
        /// </summary>
        internal bool IsVirtual
        {
            get { return Elements.Count == 1 && Elements[0].Name == null; }
        }

        public HrdNode(string name):
            base(name)
        {}

        public HrdNode():
            base(null)
        {}

        public override void AddElement(HrdElement elementBase)
        {
            if (elementBase.Name == null)
            {
                if (Elements.Count != 0)
                    throw new HrdStructureValidationException(SR.GetString(SR.UnnamedElementCantBeAdded));
            }
            else if (IsVirtual)
                throw new HrdStructureValidationException(SR.GetString(SR.NamedElementCantBeAdded));

            base.AddElement(elementBase);
        }
    }
}
