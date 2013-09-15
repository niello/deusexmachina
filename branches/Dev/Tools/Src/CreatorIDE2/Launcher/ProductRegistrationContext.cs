using System;
using System.IO;
using Microsoft.VisualStudio.Shell;
using Microsoft.Win32;

namespace CreatorIDE.Launcher
{
    public class ProductRegistrationContext:RegistrationAttribute.RegistrationContext
    {
        private readonly Type _packageType;
        private readonly RegistryKey _configKey;

        public ProductRegistrationContext(Type packageType, RegistryKey configKey)
        {
            if (packageType == null) throw new ArgumentNullException("packageType");
            if (configKey == null) throw new ArgumentNullException("configKey");
            _packageType = packageType;
            _configKey = configKey;
        }

        public override RegistrationAttribute.Key CreateKey(string name)
        {
            return new ProductRegistrationKey(_configKey.CreateSubKey(name));
        }

        public override void RemoveKey(string name)
        {
            using (var subKey = _configKey.OpenSubKey(name, true))
            {
                if (subKey == null)
                    return;
                _configKey.DeleteSubKeyTree(name);
            }
        }

        public override void RemoveValue(string keyname, string valuename)
        {
            using (var subKey = _configKey.OpenSubKey(keyname, true))
            {
                if (subKey == null)
                    return;
                subKey.DeleteValue(valuename, false);
            }
        }

        public override void RemoveKeyIfEmpty(string name)
        {
            throw new NotSupportedException();
        }

        public override string EscapePath(string str)
        {
            throw new NotSupportedException();
        }

        public override string CodeBase
        {
            get { return ComponentPath; }
        }

        public override string ComponentPath
        {
            get { return _packageType.Assembly.Location; }
        }

        public override Type ComponentType
        {
            get { return _packageType; }
        }

        public override string InprocServerPath
        {
            get { return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "mscoree.dll"); }
        }

        public override TextWriter Log
        {
            get { return Console.Out; }
        }

        public override RegistrationMethod RegistrationMethod
        {
            get { return RegistrationMethod.CodeBase; }
        }

        public override string RootFolder
        {
            get { return Path.GetDirectoryName(ComponentPath); }
        }
    }
}
