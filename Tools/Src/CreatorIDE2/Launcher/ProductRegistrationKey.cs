using System;
using Microsoft.VisualStudio.Shell;
using Microsoft.Win32;

namespace CreatorIDE.Launcher
{
    public class ProductRegistrationKey : RegistrationAttribute.Key
    {
        private readonly RegistryKey _registryKey;

        public ProductRegistrationKey(RegistryKey registryKey)
        {
            if (registryKey == null) throw new ArgumentNullException("registryKey");
            _registryKey = registryKey;
        }

        public override void Close()
        {
            _registryKey.Close();
        }

        public override RegistrationAttribute.Key CreateSubkey(string name)
        {
            var subKey = _registryKey.CreateSubKey(name, RegistryKeyPermissionCheck.ReadWriteSubTree);
            return new ProductRegistrationKey(subKey);
        }

        public override void SetValue(string valueName, object value)
        {
            object realValue = value;
            if (realValue is short)
                realValue = (int) (short) realValue;

            var regKeyName = string.IsNullOrEmpty(valueName) ? null : valueName;
            var regValue = _registryKey.GetValue(regKeyName, null);
            if (regValue == null)
            {
                if (realValue == null)
                    return;

                Console.WriteLine(@"Adding '{0}' value.", regKeyName ?? "@");
                _registryKey.SetValue(regKeyName, realValue);
            }
            else if (realValue == null)
            {
                Console.WriteLine(@"Removing '{0}' value.", regKeyName ?? "@");
                if (regKeyName == null)
                    _registryKey.SetValue(null, string.Empty);
                else
                    _registryKey.DeleteValue(regKeyName, true);
            }
            else if (!Equals(regValue, realValue))
            {
                Console.WriteLine(@"Changing '{0}' value.", regKeyName ?? "@");
                _registryKey.SetValue(regKeyName, realValue);
            }
        }
    }
}
