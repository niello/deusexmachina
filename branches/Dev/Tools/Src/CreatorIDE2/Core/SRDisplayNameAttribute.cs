using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;

namespace CreatorIDE.Core
{
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Property | AttributeTargets.Field, Inherited = false, AllowMultiple = false)]
    public sealed class SRDisplayNameAttribute : DisplayNameAttribute
    {
        private readonly string _name;
        private readonly Guid _typeID;

        public SRDisplayNameAttribute(string typeID, string name)
        {
            _name = name;
            _typeID = new Guid(typeID);
        }

        public override string DisplayName
        {
            get
            {
                string result = CideResourceManager.GetString(_typeID, _name, CultureInfo.CurrentUICulture);
                Debug.Assert(result != null, String.Format(@"String resource '{0}' is missing", _name));
                return result ?? _name;
            }
        }
    }
}
