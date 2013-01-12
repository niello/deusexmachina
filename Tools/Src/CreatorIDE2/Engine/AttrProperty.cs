using System;

namespace CreatorIDE.Engine
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
    }

    public class AttrProperty
    {
        private readonly IAttrEditorProvider _provider;
        private AttrID _attrID;
        private readonly AttrDesc _desc;
        private object _valueObj;
        private bool _modified;

        public AttrProperty(AttrID id, AttrDesc desc, CideEngine engine)
        {
            if (engine == null)
                throw new ArgumentException("engine");

            _attrID = id;
            _desc = desc;
            _valueObj = ReadFromAttr(engine);
            _provider = engine.AttrEditorProvider;
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

        public Type Type
        {
            get
            {
                switch(_attrID.Type)
                {
                    case EDataType.Bool:
                        return typeof (bool);
                    case EDataType.Float:
                        return typeof (float);
                    case EDataType.Int:
                        return typeof (int);
                    case EDataType.Matrix44:
                        return typeof (Matrix44Ref);
                    case EDataType.String:
                    case EDataType.StrID:
                        return typeof (string);
                    case EDataType.Vector4:
                        return typeof (Vector4);

                    case EDataType.Array:
                    case EDataType.Blob:
                    case EDataType.Params:
                        return typeof (object);

                    default:
                        throw new NotSupportedException(SR.GetFormatString(SR.ValueNotSupportedFormat, _attrID.Type));
                }
            }
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

        public IAttrEditorProvider EditorProvider { get { return _provider; } }

        public void ClearModified() { _modified = false; }

        private object ReadFromAttr(CideEngine engine)
        {
            switch (AttrID.Type)
            {
                case EDataType.Bool: 
                    return engine.GetBool(AttrID.ID);

                case EDataType.Int: 
                    return engine.GetInt(AttrID.ID);

                case EDataType.Float: 
                    return engine.GetFloat(AttrID.ID);

                case EDataType.String: 
                    return engine.GetString(AttrID.ID);

                case EDataType.StrID: 
                    return engine.GetStrID(AttrID.ID);

                case EDataType.Vector4: 
                    return engine.GetVector4(AttrID.ID);

                case EDataType.Matrix44: 
                    return   engine.GetMatrix44(AttrID.ID);

                default:
                    return null;
            }
        }

        private void WriteToAttr()
        {
            throw new NotImplementedException();
            //switch (AttrID.Type)
            //{
            //    case EDataType.Bool: Entities.SetBool(AttrID.ID, (bool)Value); break;
            //    case EDataType.Int: Entities.SetInt(AttrID.ID, (int)Value); break;
            //    case EDataType.Float: Entities.SetFloat(AttrID.ID, (float)Value); break;
            //    case EDataType.String: Entities.SetString(AttrID.ID, (string)Value); break;
            //    case EDataType.StrID: Entities.SetStrID(AttrID.ID, (string)Value); break;
            //    case EDataType.Vector4: Entities.SetVector4(AttrID.ID, (Vector4)Value); break;
            //    case EDataType.Matrix44: Entities.SetMatrix44(AttrID.ID, (Matrix44Ref)Value); break;
            //}
        }
    }
}
