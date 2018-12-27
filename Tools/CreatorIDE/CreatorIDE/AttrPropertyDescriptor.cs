using System;
using System.Collections.Generic;
using System.ComponentModel;
using CreatorIDE.EngineAPI;

namespace CreatorIDE
{
    public class AttrDesc
    {
        public string Category { get; set; }
        public string Description { get; set; }
        public bool IsReadOnly { get; set; }
        public bool ShowInList { get; set; }
        public bool InstanceOnly { get; set; }
        public string ResourceDir { get; set; }
        public string ResourceExt { get; set; }

        public AttrDesc()
        {
            Category = "Атрибуты";
            Description = "Здесь будет описание свойства";
            ShowInList = true;
            ResourceDir = ResourceExt = string.Empty;
        }

        // Static, stores parsed HRD
        public static Dictionary<string, AttrDesc> DescList;
    }

    class AttrProperty
    {
        private AttrID _attrID;
        private readonly AttrDesc _desc;
        private object _valueObj;
        private bool _modified;

		public AttrProperty(AttrID id, AttrDesc desc)
		{
			_attrID = id;
            _desc = desc;
            _modified = false;
		}

        public AttrProperty(AttrID id, AttrDesc desc, object value)
		{
			_attrID = id;
            _desc = desc;
            _valueObj = value;
            _modified = false;
        }

        public AttrID AttrID
        {
            get { return _attrID; }
        }

        public string Name
        {
            get { return _attrID.Name; }
        }

        public object Value
        {
            //!!!can get values here directly!
            get { return _valueObj; }
            set
            {
                //!!!can store copy of initial value & compare with it!
                if (!_modified && _valueObj != value) _modified = true;
                _valueObj = value;
            }
        }

        public string Description
        {
            get { return _desc.Description; }
        }

        public string Category
        {
            get { return _desc.Category; }
        }

        public bool ReadOnly
        {
            get { return _desc.IsReadOnly; }
        }

        public bool ShowInList
        {
            get { return _desc.ShowInList; }
        }

        public bool InstanceOnly
        {
            get { return _desc.InstanceOnly; }
        }

        public bool IsModified
        {
            get { return _modified; }
        }

        public bool IsResourceProp
        {
            get { return _desc.ResourceExt.Length > 0; }
        }

        public string ResourceDir
        {
            get { return _desc.ResourceDir; }
        }

        public string ResourceExt
        {
            get { return _desc.ResourceExt; }
        }

		public void ClearModified() { _modified = false; }

		public void ReadFromAttr()
		{
			switch (AttrID.Type)
			{
				case EDataType.Bool: Value = Entities.GetBool(AttrID.ID); break;
				case EDataType.Int: Value = Entities.GetInt(AttrID.ID); break;
				case EDataType.Float: Value = Entities.GetFloat(AttrID.ID); break;
				case EDataType.String: Value = Entities.GetString(AttrID.ID); break;
				case EDataType.StrID: Value = Entities.GetStrID(AttrID.ID); break;
				case EDataType.Vector4: Value = Entities.GetVector4(AttrID.ID); break;
				case EDataType.Matrix44: Value = Entities.GetMatrix44(AttrID.ID); break;
			}
		}

		public void WriteToAttr()
		{
			switch (AttrID.Type)
			{
				case EDataType.Bool: Entities.SetBool(AttrID.ID, (bool)Value); break;
				case EDataType.Int: Entities.SetInt(AttrID.ID, (int)Value); break;
				case EDataType.Float: Entities.SetFloat(AttrID.ID, (float)Value); break;
				case EDataType.String: Entities.SetString(AttrID.ID, (string)Value); break;
				case EDataType.StrID: Entities.SetStrID(AttrID.ID, (string)Value); break;
				case EDataType.Vector4: Entities.SetVector4(AttrID.ID, (Vector4)Value); break;
				case EDataType.Matrix44: Entities.SetMatrix44(AttrID.ID, (Matrix44Ref)Value); break;
			}
		}
	}

    class AttrPropertyDescriptor: PropertyDescriptor
	{
        private readonly AttrProperty _prop;
       
        public AttrPropertyDescriptor(AttrProperty prop, Attribute[] attrs): base(prop.Name, attrs)
		{
			_prop = prop;
		}
		
		public override bool CanResetValue(object component)
		{
			return false;
		}

		public override Type ComponentType
		{
			get { return null; }
		}

		public override object GetValue(object component)
		{
			return _prop.Value;
		}

		public override string Description
		{
			get { return _prop.Description; }
		}
		
		public override string Category
		{
            get { return _prop.Category; }
		}

		public override string DisplayName
		{
			get { return _prop.Name; }			
		}	

		public override bool IsReadOnly
		{
			get { return _prop.ReadOnly; }
		}

        public override void ResetValue(object component)
		{
            //Have to implement
            //???use default values?
            int IDoNothing = 0;
            IDoNothing++;
		}

		public override bool ShouldSerializeValue(object component)
		{
			return false;
		}

		public override void SetValue(object component, object value)
		{
			_prop.Value = value;
		}

		public override Type PropertyType
		{
			get { return _prop.Value.GetType(); }
		}

        public override object GetEditor(Type editorBaseType)
        {
            if (_prop.IsResourceProp) return new ResourceStringEditor(_prop.ResourceDir, _prop.ResourceExt);
            return base.GetEditor(editorBaseType);
        }
	}
}
