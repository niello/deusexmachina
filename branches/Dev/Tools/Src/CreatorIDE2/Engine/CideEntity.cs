using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;

namespace CreatorIDE.Engine
{
    public delegate void EntityRenamedCallback(CideEntity ent, string oldUid);

    public class CideEntity : CustomTypeDescriptor
    {
        private readonly Category _category;
        private readonly List<AttrProperty> _attrProps;
        private readonly AttrProperty _guidProp;
        private bool _exists;

        public event EntityRenamedCallback EntityRenamed;

        public string UID { get; private set; }

        //???
        public string GuidPropValue
        {
            get { return (string)_guidProp.Value; }
        }

        public string Category
        {
            get { return _category.Name; }
        }

        public CideEntity(CideEngine engine, Category category):
            this(engine, category, null, false)
        {
        }

        public CideEntity(CideEngine engine, Category category, string uid, bool exists)
        {
            if(engine==null)
                throw new ArgumentNullException("engine");
            if(category==null)
                throw new ArgumentNullException("category");

            _exists = !string.IsNullOrEmpty(uid) && exists;
            UID = uid;
            _category = category;
            _attrProps = new List<AttrProperty>();
            foreach (var id in _category.AttrIDs)
            {
                var desc = engine.GetAttrDesc(id.Name) ?? new AttrDesc();
                _attrProps.Add(new AttrProperty(id, desc, engine));
                if (id.Name == "GUID") _guidProp = _attrProps.Last();
            }
        }

       /* public bool Create(string levelID)
        {
            if (_guidProp == null || ((string)_guidProp.Value).Length < 1 || _isExists) return false;
            if (Entities.Create((string)_guidProp.Value, _cat.Name))
            {
                UID = (string)_guidProp.Value;
                SaveChangedAttrs();
                Entities.SetString(Categories.GetAttrID("LevelID").ID, levelID);
                _isExists = Entities.AttachToWorld();
                return _isExists;
            }
            return false;
        }

        public bool CreateFromTemplate(string tplName, string levelID, bool atMousePosition)
        {
            if (UID.Length < 1 || _isExists) return false;
            if (Entities.CreateFromTemplate(UID, _cat.Name, tplName))
            {
                _guidProp.Value = UID;
                Entities.SetStrID(_guidProp.AttrID.ID, UID);
                Entities.SetString(Categories.GetAttrID("LevelID").ID, levelID);
                _isExists = Entities.AttachToWorld();
                if (!_isExists) return false;
                if (atMousePosition)
                {
                    Engine.SelectEntity(UID);
                    Transform.PlaceUnderMouse();
                }
                //Update();
                return true;
            }
            return false;
        }

        public void Update()
        {
            Entities.SetCurrentEntity(UID);
            foreach (var prop in _attrProps)
            {
                prop.ReadFromAttr();
                prop.ClearModified();

                //!!!or special GUIDPropertyDescriptor.SetValue!
                //or catch onvaluechanged for this prop
                if (prop == _guidProp && prop.Value as string != UID)
                {
                    UID = prop.Value as string;
                    if (UINode != null) UINode.Text = UID ?? string.Empty;
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

        public bool Rename(string newUid) { return (_isExists) ? InternalRename(newUid, true) : false; }

        private bool InternalRename(string newUid, bool setCurrent)
        {
            if (UID == newUid) return true;
            if (setCurrent) Entities.SetCurrentEntity(UID);
            if (Entities.SetUID(UID, newUid))
            {
                string oldUid = UID;
                UID = newUid;
                if (_guidProp != null)
                {
                    _guidProp.Value = UID;
                    _guidProp.ClearModified();
                }
                if (UINode != null) UINode.Text = UID;
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
            if (_isExists) Entities.SetCurrentEntity(UID);
            bool smthChanged = false;
            foreach (var prop in _attrProps.Where(prop => prop.IsModified))
            {
                smthChanged = true;

                if (prop == _guidProp) InternalRename(prop.Value as string, false);
                else prop.WriteToAttr();

                prop.ClearModified();
            }

            return smthChanged;
        }*/

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
            return _guidProp != null
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
