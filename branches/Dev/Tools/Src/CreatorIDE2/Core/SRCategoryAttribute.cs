using System;
using System.ComponentModel;
using System.Globalization;

namespace CreatorIDE.Core
{
    [AttributeUsage(AttributeTargets.All)]
    public class SRCategoryAttribute:CategoryAttribute
    {
        private readonly Guid _typeID;

        public SRCategoryAttribute(string typeID, string category):
            base(category)
        {
            _typeID = new Guid(typeID);
        }

        protected override string GetLocalizedString(string value)
        {
            return CideResourceManager.GetString(_typeID, value, CultureInfo.CurrentCulture);
        }
    }
}
