namespace HrdLib
{
    public static class HrdElementExtension
    {
        public static bool TryGetAttributeValue<T>(this HrdElement element, string attributeName, out T attrValue)
        {
            if (element == null || attributeName == null)
            {
                attrValue = default(T);
                return false;
            }

            var attr = element.GetElement<HrdAttribute>(attributeName);
            if (attr == null || attr.Value == null)
            {
                attrValue = default(T);
                return false;
            }

            if (attr.Value is T)
            {
                attrValue = (T)attr.Value;
                return true;
            }

            attrValue = default(T);
            return false;
        }

        public static bool TryGetArrayElement<T>(this HrdArray array, int index, out T value)
        {
            if (index < 0 || index >= array.Count)
            {
                value = default(T);
                return false;
            }

            var attr = array[index] as HrdAttribute;

            if (attr == null || attr.Value == null)
            {
                value = default(T);
                return false;
            }

            if (attr.Value is T)
            {
                value = (T)attr.Value;
                return true;
            }

            value = default(T);
            return false;
        }
    }
}
