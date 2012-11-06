using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;

namespace CreatorIDE.Package
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Property | AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    internal sealed class SRDisplayNameAttribute : DisplayNameAttribute
    {
        readonly string _name;

        public SRDisplayNameAttribute(string name)
        {
            _name = name;
        }

        public override string DisplayName
        {
            get
            {
                string result = SR.GetString(_name, CultureInfo.CurrentUICulture);
                Debug.Assert(result != null, String.Format(@"String resource '{0}' is missing", _name));
                return result ?? _name;
            }
        }
    }
}
