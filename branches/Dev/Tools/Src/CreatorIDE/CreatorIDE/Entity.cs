using System;
using System.Collections.Generic;
using System.Linq;
using System.ComponentModel;
using System.Windows.Forms;
using CreatorIDE.EngineAPI;

namespace CreatorIDE
{
	public delegate void EntityRenamedCallback(Entity ent, string oldUid);

    public class Entity : CustomTypeDescriptor
    {
        private string _UID;
        private readonly Category _cat;
        private readonly List<AttrProperty> _attrProps;
        private AttrProperty _guidProp;
        private bool _isExisting;

        public event EntityRenamedCallback EntityRenamed;

        public TreeNode UINode { get; set; }

        public string UID
        {
            get { return _UID; }
        }

        //???
        public string GuidPropValue
        {
            get { return (string)_guidProp.Value; }
        }

        public string Category
        {
            get { return _cat.Name; }
        }

		public Entity(string uid, Category cat, bool existing)
        {
			_isExisting = (uid.Length > 0) && existing;
            _UID = uid;
            _cat = cat;
            _attrProps = new List<AttrProperty>();
            foreach (var id in _cat.AttrIDs)
            {
                AttrDesc desc;
                if (!AttrDesc.DescList.TryGetValue(id.Name, out desc))
                    desc = new AttrDesc();
                _attrProps.Add(new AttrProperty(id, desc));
                if (id.Name == "GUID") _guidProp = _attrProps.Last();
           }
        }

        public bool Create(string levelID)
        {
            if (_guidProp == null || ((string)_guidProp.Value).Length < 1 || _isExisting) return false;
            if (Entities.Create((string)_guidProp.Value, _cat.Name))
            {
                _UID = (string)_guidProp.Value;
                SaveChangedAttrs();
                Entities.SetString(Categories.GetAttrID("LevelID").ID, levelID);
                _isExisting = Entities.AttachToWorld();
                return _isExisting;
            }
            return false;
        }

        public bool CreateFromTemplate(string tplName, string levelID, bool atMousePosition)
        {
            if (_UID.Length < 1 || _isExisting) return false;
            if (Entities.CreateFromTemplate(_UID, _cat.Name, tplName))
            {
                _guidProp.Value = _UID;
                Entities.SetStrID(_guidProp.AttrID.ID, _UID);
                Entities.SetString(Categories.GetAttrID("LevelID").ID, levelID);
                _isExisting = Entities.AttachToWorld();
                if (!_isExisting) return false;
                if (atMousePosition)
                {
                    Engine.SelectEntity(_UID);
                    Transform.PlaceUnderMouse();
                }
                //Update();
                return true;
            }
            return false;
        }

		public void Update()
		{
			Entities.SetCurrentEntity(_UID);
			foreach (var prop in _attrProps)
			{
				prop.ReadFromAttr();
				prop.ClearModified();

				//!!!or special GUIDPropertyDescriptor.SetValue!
				//or catch onvaluechanged for this prop
				if (prop == _guidProp && prop.Value as string != _UID)
				{
					_UID = prop.Value as string;
                    if (UINode != null) UINode.Text = _UID ?? string.Empty;
				}
			}
		}

		public void Save()
		{
			if (SaveChangedAttrs()) Entities.UpdateAttrs();
		}

		public bool SaveAsTemplate(string templateUid)
		{
            bool result = false;
            if (Entities.BeginTemplate(templateUid, _cat.Name, true))
            {
                foreach (var prop in _attrProps.Where(prop => !prop.InstanceOnly && prop != _guidProp))
                    prop.WriteToAttr();
                result = true;
            }
		    Entities.EndTemplate();
			return result;
		}

		public bool Rename(string newUid) { return (_isExisting) ? InternalRename(newUid, true): false; }

		private bool InternalRename(string newUid, bool setCurrent)
		{
            if (_UID == newUid) return true;
			if (setCurrent) Entities.SetCurrentEntity(_UID);
			if (Entities.SetUID(_UID, newUid))
			{
                string oldUid = _UID;
				_UID = newUid;
				if (_guidProp != null)
				{
					_guidProp.Value = _UID;
					_guidProp.ClearModified();
				}
				if (UINode != null) UINode.Text = _UID;
                OnEntityRenamed(oldUid);
				return true;
			}
			return false;
		}

        private void OnEntityRenamed(string oldUid)
        {
            var h = EntityRenamed;
            if (h != null) h(this, oldUid);
        }

		protected bool SaveChangedAttrs()
		{
			if (_isExisting) Entities.SetCurrentEntity(_UID);
			bool smthChanged = false;
			foreach (var prop in _attrProps.Where(prop => prop.IsModified))
			{
			    smthChanged = true;
			    
                if (prop == _guidProp) InternalRename(prop.Value as string, false);
			    else prop.WriteToAttr();

			    prop.ClearModified();
			}

			return smthChanged;
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
            return (_guidProp != null)
                       ? new AttrPropertyDescriptor(_guidProp, new Attribute[0])
                       : null;
        }

        public override object GetPropertyOwner(PropertyDescriptor pd)
        {
            return this;
		}

		#endregion
	}
}
