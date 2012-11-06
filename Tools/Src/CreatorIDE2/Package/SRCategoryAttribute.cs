using System;
using System.ComponentModel;
using System.Globalization;

namespace CreatorIDE.Package
{
    [AttributeUsage(AttributeTargets.All)]
    internal class SRCategoryAttribute:CategoryAttribute 
    {
        public SRCategoryAttribute(string category):
            base(category)
        {}

        protected override string GetLocalizedString(string value)
        {
            return SR.GetString(value, CultureInfo.CurrentCulture);
        }
    }
}
