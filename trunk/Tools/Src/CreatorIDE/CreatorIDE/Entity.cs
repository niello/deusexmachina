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
        private string _uid;
        private readonly Category _cat;
        private readonly List<AttrProperty> _attrProps;
        private AttrProperty _guidProp;
        private bool _isExisting;

        public event EntityRenamedCallback EntityRenamed;

        public TreeNode UiNode { get; set; }

        public string Uid
        {
            get { return _uid; }
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
            _uid = uid;
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
                _uid = (string)_guidProp.Value;
                SaveChangedAttrs();
                Entities.SetString(Categories.GetAttrID("LevelID").ID, levelID);
                _isExisting = Entities.AttachToWorld();
                return _isExisting;
            }
            return false;
        }

        public bool CreateFromTemplate(string tplName, string levelID, bool atMousePosition)
        {
            if (_uid.Length < 1 || _isExisting) return false;
            if (Entities.CreateFromTemplate(_uid, _cat.Name, tplName))
            {
                _guidProp.Value = _uid;
                Entities.SetStrID(_guidProp.AttrID.ID, _uid);
                Entities.SetString(Categories.GetAttrID("LevelID").ID, levelID);
                _isExisting = Entities.AttachToWorld();
                if (!_isExisting) return false;
                if (atMousePosition)
                {
                    Transform.SetCurrentEntity(_uid);
                    Transform.PlaceUnderMouse();
                }
                //Update();
                return true;
            }
            return false;
        }

		public void Update()
		{
			Entities.SetCurrentEntity(_uid);
			foreach (var prop in _attrProps)
			{
				prop.ReadFromAttr();
				prop.ClearModified();

				//!!!or special GUIDPropertyDescriptor.SetValue!
				//or catch onvaluechanged for this prop
				if (prop == _guidProp && prop.Value as string != _uid)
				{
					_uid = prop.Value as string;
                    if (UiNode != null) UiNode.Text = _uid ?? string.Empty;
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
            if (_uid == newUid) return true;
			if (setCurrent) Entities.SetCurrentEntity(_uid);
			if (Entities.SetUID(_uid, newUid))
			{
                string oldUid = _uid;
				_uid = newUid;
				if (_guidProp != null)
				{
					_guidProp.Value = _uid;
					_guidProp.ClearModified();
				}
				if (UiNode != null) UiNode.Text = _uid;
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
			if (_isExisting) Entities.SetCurrentEntity(_uid);
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
