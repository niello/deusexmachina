using System.Xml.Linq;

namespace DialogLogic
{
    public static class XElementExensions
    {
        public static bool TryGetAttribute(this XElement element, string attributeName, out bool boolVal)
        {
            boolVal = default(bool);
            string str;

            return TryGetAttributeString(element, attributeName, out str) && bool.TryParse(str, out boolVal);
        }

        public static bool TryGetAttribute(this XElement element, string attributeName, out int intVal)
        {
            intVal = default(int);
            string str;

            return TryGetAttributeString(element, attributeName, out str) && int.TryParse(str, out intVal);
        }

        public static bool TryGetAttribute(this XElement element, string attributeName, out string strVal)
        {
            return TryGetAttributeString(element, attributeName, out strVal);
        }

        private static bool TryGetAttributeString(XElement element, string attributeName, out string attrString)
        {
            attrString = null;
            if(element==null)
                return false;

            var attr=element.Attribute(attributeName);
            if(attr==null)
                return false;

            if(string.IsNullOrEmpty(attr.Value))
                return false;

            attrString = attr.Value;
            return true;
        }

        public static XElement AddOrClearChild(this XElement element, string childElementName)
        {
            var child = element.Element(childElementName);
            if(child==null)
            {
                child = new XElement(childElementName);
                element.Add(child);
            }
            else
                child.RemoveAll();
            return child;
        }
    }
}
