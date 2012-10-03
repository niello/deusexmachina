using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Xml.Linq;

namespace DialogLogic
{
    public class CharacterAttributeCollection
    {
        private readonly ICharacterContainer _characterContainer;

        private readonly List<PropertyDescriptor> _currentLevelProperties = new List<PropertyDescriptor>();

        private readonly PropertyDescriptorCollection _properties =
            new PropertyDescriptorCollection(new PropertyDescriptor[0], false);

        private event Action<PropertyDescriptor> PropertyAdded;
        internal event Action<PropertyDescriptor> CurrentLevelPropertyAdded;

        public CharacterAttributeCollection(ICharacterContainer container)
        {
            _characterContainer = container;
            var coll = _characterContainer.ParentContainer == null
                           ? null
                           : _characterContainer.ParentContainer.AttributeCollection;

            if(coll!=null)
            {
                foreach (PropertyDescriptor prop in coll._properties)
                    AddProperty(prop, false);

                coll.PropertyAdded += OnBasePropertyAdded;
            }
        }

        private void AddProperty(PropertyDescriptor descriptor, bool currentLevel)
        {
            _properties.Add(descriptor);
            if (currentLevel)
                _currentLevelProperties.Add(descriptor);

            if(descriptor is DialogCharacterPropertyDescriptor)
                ((DialogCharacterPropertyDescriptor) descriptor).IsReadOnlyRequest += ReadOnlyRequest;

            OnPropertyAdded(descriptor);
            if (currentLevel)
                OnCurrentLevelPropertyAdded(descriptor);
        }

        private void ReadOnlyRequest(object sender, IsReadOnlyRequestEArgs e)
        {
            if (e.HandledBy != null)
            {
                e.IsReadOnly = true;
                e.HandledBy = _characterContainer;
            }
            e.IsReadOnly = false;
        }

        private void OnBasePropertyAdded(PropertyDescriptor obj)
        {
            AddProperty(obj, false);
        }

        public PropertyDescriptorCollection GetProperties()
        {
            return _properties;
        }

        public PropertyDescriptorCollection GetProperties(Attribute[] attributes)
        {
            return
                new PropertyDescriptorCollection(
                    (from PropertyDescriptor descr in _properties
                     where descr.Attributes.Contains(attributes)
                     select descr).ToArray());
        }

        public void AddProperty(string propertyName, Type propertyType)
        {
            var descr = new DialogCharacterPropertyDescriptor(propertyName, propertyType, new Attribute[0]);
            AddProperty(descr, true);
        }

        public void AddProperty(DialogCharacterPropertyDescriptor descr)
        {
            AddProperty(descr, true);
        }

        private void OnPropertyAdded(PropertyDescriptor descr)
        {
            if (PropertyAdded != null)
                PropertyAdded(descr);
        }

        private void OnCurrentLevelPropertyAdded(PropertyDescriptor descr)
        {
            if (CurrentLevelPropertyAdded != null)
                CurrentLevelPropertyAdded(descr);
        }

        internal List<PropertyDescriptor> GetAssociatedProperties(ICharacterContainer characterContainer)
        {
            if(characterContainer==_characterContainer)
                return _currentLevelProperties;

            if (_characterContainer.ParentContainer != null)
                return
                    _characterContainer.ParentContainer.AttributeCollection.GetAssociatedProperties(characterContainer);

            return null;
        }

        public int GetPropertyDepth(string name)
        {
            int res = -1;
            if(_currentLevelProperties.Any(pr=>pr.Name==name))
            {
                res = 0;
            }
            else if(_characterContainer.ParentContainer!=null)
            {
                res = _characterContainer.ParentContainer.AttributeCollection.GetPropertyDepth(name);
                if (res >= 0)
                    res++;
            }

            return res;
        }

        public Dictionary<string ,int > WriteXml(XElement xProperties)
        {
            var res = new Dictionary<string, int>();
            int id = 0;
            foreach(var descr in _currentLevelProperties)
            {
                var xProperty = new XElement("property");
                res.Add(descr.Name, ++id);

                xProperty.Add(new XAttribute("id", id), new XAttribute("name", descr.Name),
                                new XAttribute("type", descr.PropertyType.ToString()));

                if(descr is DialogCharacterPropertyDescriptor && ((DialogCharacterPropertyDescriptor)descr).DefaultValue!=null)
                    xProperty.SetValue(((DialogCharacterPropertyDescriptor)descr).DefaultValue);

                xProperties.Add(xProperty);
            }
            return res;
        }

        public Dictionary<int,string> ReadXml(XElement xProperties, List<string> ignoredProperties)
        {
            var res = new Dictionary<int, string>();
            foreach(var xProperty in xProperties.Elements("property"))
            {
                string name,typeStr;
                int id;
                if (!xProperty.TryGetAttribute("name", out name) || !xProperty.TryGetAttribute("id", out id) || !xProperty.TryGetAttribute("type", out typeStr))
                    throw new Exception("Unable to read character's property description: format is incorrect.");

                res.Add(id, name);
                if(ignoredProperties.Contains(name))
                    continue;

                var type = Type.GetType(typeStr);

                DialogCharacterPropertyDescriptor descriptor;
                object defaultValue;
                if (ValueParser.TryParse(xProperty.Value, type, out defaultValue))
                    descriptor = new DialogCharacterPropertyDescriptor(name, type, defaultValue, new Attribute[0]);
                else
                    descriptor = new DialogCharacterPropertyDescriptor(name, type, new Attribute[0]);

                AddProperty(descriptor);
            }

            return res;
        }

        public Type GetPropertyType(string name)
        {
            return _properties[name].PropertyType;
        }

        public bool ContainsProperty(string propName, bool onlyCurrentLevel)
        {
            return onlyCurrentLevel
                       ? _currentLevelProperties.Any(prop => prop.Name == propName)
                       : _properties.Cast<PropertyDescriptor>().Any(prop => prop.Name == propName);
        }
    }
}
