using System.Globalization;
using System.Resources;
using System.Threading;

namespace HrdLib
{
    internal static class SR
    {
        public const string
            AmbigousConstructorDeclaredFormat = "AmbigousConstructorDeclaredFormat",
            CompilerErrorCollectionEmpty = "CompilerErrorCollectionEmpty",
            IncorrectOrderFormat = "IncorrectOrderFormat",
            InternalCodeGenError = "InternalCodeGenError",
            NamedElementCantBeAdded = "NamedElementCantBeAdded",
            NoApproppriateConstructorFormat = "NoApproppriateConstructorFormat",
            NoPublicPropertyFormat = "NoPublicPropertyFormat",
            NonAttributeValueCantBeRead = "NonAttributeValueCantBeRead",
            ParameterSetterAlreadyDeclaredFormat = "ParameterSetterAlreadyDeclaredFormat",
            ParameterTypeMismatchFormat = "ParameterTypeMismatchFormat",
            PropertyCantBeNullFormat = "PropertyCantBeNullFormat",
            TypeCantBeGenericDefinitionFormat = "TypeCantBeGenericDefinitionFormat",
            TypeCantBeInterfaceFormat = "TypeCantBeInterfaceFormat",
            TypeMarkedNonserializableFormat = "TypeMarkedNonserializableFormat",
            UnnamedElementCantBeAdded = "UnnamedElementCantBeAdded",
            ValueTypeMismatch = "ValueTypeMismatch";

        private static ResourceManager _resourceManager;
        private static ResourceManager ResourceManager
        {
            get
            {
                if (_resourceManager == null)
                {
                    var mgr = new ResourceManager("HrdLib.Resources", typeof(SR).Assembly);
                    return Interlocked.CompareExchange(ref _resourceManager, mgr, null) ?? mgr;
                }
                return _resourceManager;
            }
        }
        
        public static string GetString(string name)
        {
            return ResourceManager.GetString(name, CultureInfo.CurrentCulture) ?? name;
        }

        public static string GetString(CultureInfo cultureInfo, string name)
        {
            return ResourceManager.GetString(name, cultureInfo) ?? name;
        }

        public static string GetFormatString(string name, params object[] args)
        {
            var formatString = ResourceManager.GetString(name, CultureInfo.CurrentCulture);
            return formatString == null ? name : string.Format(formatString, args);
        }

        public static string GetFormatString(CultureInfo cultureInfo, string name, params object[] args)
        {
            var formatString = ResourceManager.GetString(name, cultureInfo);
            return formatString == null ? name : string.Format(formatString, args);
        }
    }
}
