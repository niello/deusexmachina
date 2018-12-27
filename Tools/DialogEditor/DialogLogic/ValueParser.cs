using System;

namespace DialogLogic
{
    public static class ValueParser
    {
        public static PropertyTypeCode GetTypeCode(Type type)
        {
            var tc=Type.GetTypeCode(type);
            var strCode = Enum.GetName(typeof (System.TypeCode), tc);
            if (Enum.IsDefined(typeof(PropertyTypeCode), strCode))
                return (PropertyTypeCode) Enum.Parse(typeof (PropertyTypeCode), strCode);
            return PropertyTypeCode.Unknown;
        }

        public static bool TryParse(string str, PropertyTypeCode code, out object value)
        {
            bool res;
            switch(code)
            {
                case PropertyTypeCode.Boolean:
                    bool boolVal;
                    res = bool.TryParse(str, out boolVal);
                    value = boolVal;
                    return res;

                case PropertyTypeCode.Int32:
                    int intVal;
                    res = int.TryParse(str, out intVal);
                    value = intVal;
                    return res;

                case PropertyTypeCode.String:
                    value = str;
                    return str != null;
                    
                default:
                    throw new Exception(string.Format("Type code {0} is not supported.", code));
            }
        }

        public static bool TryParse(string str, Type type, out object value)
        {
            return TryParse(str, GetTypeCode(type), out value);
        }
    }
}
