using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.Xml.Linq;

namespace DialogLogic
{
    public partial class DialogCharacter:IComparable<DialogCharacter>,INotifyPropertyChanged
    {
        private readonly DialogCharacter _baseCharacter;
        private readonly Dictionary<string, object> _properties = new Dictionary<string, object>();
        private readonly ICharacterContainer _characterContainer;

        public event PropertyChangedEventHandler PropertyChanged;
        
        public string Name
        {
            get { return GetValue<string>("Name"); }
            set { SetValue("Name", value); }
        }
        
        public bool IsPlayer
        {
            get { return GetValue<bool>("IsPlayer"); }
            set { SetValue("IsPlayer", value); }
        }

        public int Id
        {
            get { return GetValue<int>("Id"); }
            set { SetValue("Id", value); }
        }

        public DialogCharacter(ICharacterContainer container):
            this(null,container)
        {}

        public DialogCharacter(DialogCharacter baseCharacter, ICharacterContainer container)
        {
            Debug.Assert(container != null);

            if(baseCharacter!=null)
            {
                Debug.Assert(container.ParentContainer != null &&
                             baseCharacter._characterContainer == container.ParentContainer);
            }

            _characterContainer = container;

            if (baseCharacter != null)
                _baseCharacter = baseCharacter;
            else if (container.ParentContainer != null)
                _baseCharacter = new DialogCharacter(container.ParentContainer);

            var props = _characterContainer.AttributeCollection.GetAssociatedProperties(_characterContainer);
            if(props!=null && props.Count>0)
            {
                foreach(var prop in props.OfType<DialogCharacterPropertyDescriptor>())
                {
                    AddProperty(prop.Name, true);
                    if (prop.HasDefaultValue)
                        SetValue(prop.Name, prop.DefaultValue);
                }
            }
            
            if(_baseCharacter!=null)
                _baseCharacter.PropertyChanged += BaseCharacterPropertyChanged;

            container.AttributeCollection.CurrentLevelPropertyAdded += OnPropertyAdded;
        }

        private void OnPropertyAdded(PropertyDescriptor obj)
        {
            var descr = obj as DialogCharacterPropertyDescriptor;
            if(descr!=null)
            {
                AddProperty(descr.Name, false);
                if (descr.HasDefaultValue)
                    SetValue(descr.Name, descr.DefaultValue);
            }
        }

        private void BaseCharacterPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            OnPropertyChanged(e.PropertyName);
        }

        public T GetValue<T>(string propName, T defaultValue)
        {
            object res;
            if (!TryGetValueRecursive(propName, out res))
                res = defaultValue;

            return SafelyCast(res, defaultValue);
        }

        internal object GetValue(Type propertyType, string propName, object defaultValue)
        {
            object res;
            if(!TryGetValueRecursive(propName,out res))
                return defaultValue;

            return res;
        }

        public T GetValue<T>(string propName)
        {
            object res;
            if (!TryGetValueRecursive(propName, out res))
                throw new KeyNotFoundException(string.Format("Property '{0}' is not found.", propName));

            return SafelyCast(res, default(T));
        }

        private static T SafelyCast<T>(object value, T defaultValue)
        {
            return (T) SafelyCast(typeof (T), value, defaultValue);
        }

        private static object SafelyCast(Type targetType, object value, object defaultValue)
        {
            object res;
            if(value==null && targetType.IsValueType)
            {
                if (targetType.IsGenericType && targetType.GetGenericTypeDefinition() == typeof(Nullable<>))
                    res = null;
                else
                    res = defaultValue;
            }
            else
                res = value;

            return res;
        }

        private bool TryGetValueRecursive(string propName, out object value)
        {
            object res;
            if (_properties.TryGetValue(propName, out res))
            {
                value = res;
                return true;
            }

            if (_baseCharacter != null)
                return _baseCharacter.TryGetValueRecursive(propName, out value);

            value = null;
            return false;
        }

        public void SetValue(string propName, object value)
        {
            var ch = this;
            while(ch!=null)
            {
                if(ch._properties.ContainsKey(propName))
                {
                    ch._properties[propName] = value;
                    OnPropertyChanged(propName);
                    return;
                }

                ch = ch._baseCharacter;
            }

            throw new KeyNotFoundException(string.Format("Property '{0}' is not found.", propName));
        }

        public void AddProperty(string propName, bool overrideProperty)
        {
            int d = GetPropertyDepth(propName);
            if ((overrideProperty && d == 0) || (!overrideProperty && d >= 0))
                throw new ArgumentException(string.Format("Property '{0}' is already exists.", propName));

            _properties.Add(propName, null);
        }

        public bool DeleteProperty(string propName, bool deleteFromBase)
        {
            bool del = _properties.Remove(propName);
            if (!del && deleteFromBase && _baseCharacter != null)
                return _baseCharacter.DeleteProperty(propName, true);

            return del;
        }

        private int GetPropertyDepth(string propName)
        {
            var ch = this;
            int depth = 0;
            while(ch!=null)
            {
                if(ch._properties.ContainsKey(propName))
                    return depth;

                depth++;
                ch = ch._baseCharacter;
            }

            return -1;
        }

        public int CompareTo(DialogCharacter other)
        {
            return string.Compare(Name, other == null ? null : other.Name, false);
        }

        public override string ToString()
        {
            return Name ?? string.Empty;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != typeof (DialogCharacter)) return false;
            return Equals((DialogCharacter) obj);
        }

        public bool Equals(DialogCharacter other)
        {
            if (ReferenceEquals(null, other)) return false;
            if (ReferenceEquals(this, other)) return true;
            return Equals(other.Name, Name);
        }

        public override int GetHashCode()
        {
            return (Name != null ? Name.GetHashCode() : 0);
        }

        private void OnPropertyChanged(string propertyName)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
        }

        public void WriteProperties(XElement xCharacter, Func<string ,int> propertyMapping)
        {
            foreach(var prop in _properties)
            {
                var propElement = new XElement("propertyValue");
                int id = propertyMapping(prop.Key);
                propElement.Add(new XAttribute("id",id));

                if(prop.Value!=null)
                    propElement.SetValue(prop.Value);

                xCharacter.Add(propElement);
            }
        }

        public void ReadProperties(XElement xCharacter, Func<int,string> propertyMapping)
        {
            foreach (var propElement in xCharacter.Elements("propertyValue"))
            {
                int id;
                if(!propElement.TryGetAttribute("id",out id))
                    continue;

                var propName = propertyMapping(id);
                if (string.IsNullOrEmpty(propName) || !_characterContainer.AttributeCollection.ContainsProperty(propName, true))
                    continue;

                var type = _characterContainer.AttributeCollection.GetPropertyType(propName);

                object value;
                if (ValueParser.TryParse(propElement.Value, type, out value))
                    SetValue(propName, value);
            }
        }
    }
}
