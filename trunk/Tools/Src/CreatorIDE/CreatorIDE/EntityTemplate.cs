using System;
using System.Collections.Generic;
using System.Linq;
using System.ComponentModel;
using System.Windows.Forms;
using CreatorIDE.EngineAPI;

namespace CreatorIDE
{
    public class EntityTemplate : CustomTypeDescriptor
    {
        readonly Category _cat;
        readonly List<AttrProperty> _attrProps;
        readonly AttrProperty _guidProp;
        
        public TreeNode UiNode { get; set; }

        public string Uid { get; private set; }

        public string Category
        {
            get { return _cat.Name; }
        }

		public EntityTemplate(string uid, Category cat)
        {
            Uid = uid;
            _cat = cat;
            _attrProps = new List<AttrProperty>();
            foreach (var id in _cat.AttrIDs)
            {
                var desc = AttrDesc.DescList.ContainsKey(id.Name) ? AttrDesc.DescList[id.Name] : new AttrDesc();
                if (desc.InstanceOnly) continue;
                _attrProps.Add(new AttrProperty(id, desc));
                if (id.Name == "GUID") _guidProp = _attrProps.Last();
            }
        }

		public void Update()
		{
			Entities.BeginTemplate(Uid, _cat.Name, false);

			foreach (var prop in _attrProps)
			{
				prop.ReadFromAttr();
				prop.ClearModified();

			    if (prop != _guidProp || prop.Value as string == Uid) continue;
			    Uid = prop.Value as string;
                if (UiNode != null) UiNode.Text = Uid ?? string.Empty;
			}

			Entities.EndTemplate();
		}

		public bool Save()
		{
            // New template, empty UID
            if (string.IsNullOrEmpty(Uid))
            {
                Uid = _guidProp.Value as string;
                _guidProp.ClearModified();
            }

            if (!Entities.BeginTemplate(Uid, _cat.Name, true))
            {
                Entities.EndTemplate();
                return false;
            }

            // Existing template renamed //!!!SQL will not execute as expected!
            if (_guidProp.IsModified)
            {
                // Not supported for now
                _guidProp.Value = Uid;
                _guidProp.ClearModified();
                //UID = GUIDProp.Value as string;
                //if (UINode != null) UINode.Text = UID;
            }

			foreach (var prop in _attrProps.Where(prop => prop.IsModified))
			{
			    prop.WriteToAttr();
			    prop.ClearModified();
			}

			Entities.EndTemplate();

			return true;
		}

		#region CustomTypeDescriptor

		public override PropertyDescriptorCollection GetProperties(Attribute[] attrs)
        {
            var propDescs = new PropertyDescriptor[_attrProps.Count];
            for (int i = 0, count = 0; i < _attrProps.Count; i++)
            {
                var prop = _attrProps[i];
                if (prop.ShowInList) propDescs[count++] = new AttrPropertyDescriptor(prop, attrs);
            }
            return new PropertyDescriptorCollection(propDescs);
        }

        public override PropertyDescriptor GetDefaultProperty()
        {
			return (_guidProp != null) ?
				new AttrPropertyDescriptor(_guidProp, new Attribute[0]) :
				null;
        }

        public override object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
		}

		#endregion
	}
}
