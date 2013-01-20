using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    partial class CideEngine
    {
        public List<CideEntity> GetEntities()
        {
            return _entityCache.GetEntities(this);
        }

        internal int GetEntityCount()
        {
            return GetEntityCount(_engineHandle.Handle);
        }

        internal string GetEntityCategory(int entityIdx)
        {
            var category = new StringBuilder(256);
            GetEntityCategory(_engineHandle.Handle, entityIdx, category);
            return category.ToString();
        }

        internal string GetEntityUID(int entityIndex)
        {
            var uid = new StringBuilder(256);
            GetEntityUID(_engineHandle.Handle, entityIndex, uid);
            return uid.ToString();
        }

        internal bool SetCurrentEntity(string entityUID)
        {
            return SetCurrentEntity(_engineHandle.Handle, entityUID);
        }

        internal bool GetBool(int attrID)
        {
            return GetBool(_engineHandle.Handle, attrID);
        }

        internal float GetFloat(int attrID)
        {
            return GetFloat(_engineHandle.Handle, attrID);
        }

        internal int GetInt(int attrID)
        {
            return GetInt(_engineHandle.Handle, attrID);
        }

        internal Matrix44Ref GetMatrix44(int attrID)
        {
            var data = new float[16];
            GetMatrix44(_engineHandle.Handle, attrID, data);
            return new Matrix44Ref(data);
        }

        internal string GetStrID(int attrID)
        {
            var buffer = new byte[256];
            GetStrID(_engineHandle.Handle, attrID, buffer);
            var str = GetUtf8String(buffer);
            return str;
        }

        internal string GetString(int attrID)
        {
            var buffer = new byte[1024];
            GetString(_engineHandle.Handle, attrID, buffer);
            var str = GetUtf8String(buffer);
            return str;
        }

        private static string GetUtf8String(byte[] buffer)
        {
            // It's possible to get offset as a parameter
            const int offset = 0;

            int strLen = offset, len = buffer.Length;
            while (strLen < buffer.Length && buffer[strLen] != 0)
                strLen++;
            if (strLen > len)
                strLen = offset; // String is not terminated with zero
            return Encoding.UTF8.GetString(buffer, 0, strLen);
        }

        internal Vector4 GetVector4(int attrID)
        {
            var data = new float[4];
            GetVector4(_engineHandle.Handle, attrID, data);
            return new Vector4(data);
        }

        private void OnEntityPropertyChanged(object sender, CideEntityPropertyChangedEventArgs e)
        {
            string uid;
            if (e.PropertyName == CideEntity.UIDPropertyName)
                uid = (string)e.OldValue;
            else
                uid = e.Entity.UID;
            SetEntityProperty(e.PropertyName, uid, e.Entity.CategoryUID, e.NewValue);

            var action = new ActionRecord {IsCategory = false, NewValue = e.NewValue, OldValue = e.OldValue, UID = uid};
            _actionList.Push(action);

            var h = EntityPropertyChanged;
            if (h != null)
                h(this, e);
        }

        private void SetEntityProperty(string propertyName, string uid, string categoryUID, object value)
        {
            var category = GetCategory(categoryUID);
            var attr = category.AttrIDs.First(a => a.Name == propertyName);

            SetCurrentEntity(uid);
            switch (attr.Type)
            {
                case EDataType.Bool:
                    SetBool(attr.ID, (bool) value);
                    break;

                case EDataType.Int:
                    SetInt(attr.ID, (int) value);
                    break;

                case EDataType.Float:
                    SetFloat(attr.ID, (float) value);
                    break;

                case EDataType.String:
                    SetString(attr.ID, (string)value);
                    break;

                case EDataType.StrID:
                    SetStrID(attr.ID, (string) value);
                    break;

                case EDataType.Vector4:
                    SetVector4(attr.ID, (Vector4) value);
                    break;

                case EDataType.Matrix44:
                    SetMatrix44(attr.ID, (Matrix44Ref) value);
                    break;

                default:
                    throw new NotSupportedException(SR.GetFormatString(SR.ValueNotSupportedFormat, attr.Type));
            }
        }

        private void SetBool(int attrID, bool value)
        {
            SetBool(_engineHandle.Handle, attrID, value);
        }

        private void SetInt(int attrID, int value)
        {
            SetInt(_engineHandle.Handle, attrID, value);
        }

        private void SetFloat(int attrID, float value)
        {
            SetFloat(_engineHandle.Handle, attrID, value);
        }

        private void SetString(int attrID, string value)
        {
            var bytes = GetUtf8Bytes(value, 1024);
            SetString(_engineHandle.Handle, attrID, bytes);
        }

        private void SetStrID(int attrID, string value)
        {
            var bytes = GetUtf8Bytes(value, 256);
            SetStrID(_engineHandle.Handle, attrID, bytes);
        }

        private static byte[] GetUtf8Bytes(string value, int maxBufferSize)
        {
            if (value == null)
                return new byte[] {0};

            var bytes = Encoding.UTF8.GetBytes(value);
            if (bytes.Length >= maxBufferSize)
                throw new ArgumentException(SR.StringBufferExceeded, "value");

            Array.Resize(ref bytes, bytes.Length + 1);

            return bytes;
        }

        private void SetVector4(int attrID, Vector4 value)
        {
            float[] data = {value.X, value.Y, value.Z, value.W};
            SetVector4(_engineHandle.Handle, attrID, data);
        }

        private void SetMatrix44(int attrID, Matrix44Ref value)
        {
            float[] data = value.ToArray();
            SetMatrix44(_engineHandle.Handle, attrID, data);
        }

        #region DLL Import

        [DllImport(DllName, EntryPoint = "Entities_GetCount")]
        private static extern int GetEntityCount(IntPtr handle);

        [DllImport(DllName, EntryPoint = "Entities_GetUIDByIndex")]
        private static extern void GetEntityUID(
            IntPtr handle,
            int idx,
            [MarshalAs(UnmanagedType.LPStr)] StringBuilder uid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetNextFreeUIDOnLevel")]
        private static extern int GetNextFreeUIDOnLevel(string @base);

        [DllImport(DllName, EntryPoint = "Entities_GetCatByIndex")]
        private static extern void GetEntityCategory(
            IntPtr handle,
            int idx,
            [MarshalAs(UnmanagedType.LPStr)] StringBuilder category);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetCatByUID")]
        private static extern string GetCategoryByUID(string uid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetUIDUnderMouse")]
        private static extern string GetUIDUnderMouse();

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_CreateByCategory")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool Create([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid, string category);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_CreateFromTemplate")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool CreateFromTemplate([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid, string category, string tplID);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_BeginTemplate")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool BeginTemplate(string uid, string category, [MarshalAs(UnmanagedType.I1)]bool createIfNotExist);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_EndTemplate")]
        private static extern void EndTemplate();

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_AttachToWorld")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool AttachToWorld([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_DeleteByUID")]
        private static extern void DeleteEntity(string uid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_DeleteTemplate")]
        private static extern void DeleteTemplate(string uid, string category);

        [DllImport(DllName, EntryPoint = "Entities_SetCurrentByUID")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool SetCurrentEntity(IntPtr handle, string uid);

        #region Get attribute value

        [DllImport(DllName, EntryPoint = "Entities_GetBool")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool GetBool(IntPtr handle, int attrID);

        [DllImport(DllName, EntryPoint = "Entities_GetInt")]
        private static extern int GetInt(IntPtr handle, int attrID);

        [DllImport(DllName, EntryPoint = "Entities_GetFloat")]
        private static extern float GetFloat(IntPtr handle, int attrID);

        [DllImport(DllName, EntryPoint = "Entities_GetString")]
        private static extern void GetString(
            IntPtr handle,
            int attrID,
            [MarshalAs(UnmanagedType.LPArray)] byte[] val);

        [DllImport(DllName, EntryPoint = "Entities_GetStrID")]
        private static extern void GetStrID(
            IntPtr handle,
            int attrID,
            [MarshalAs(UnmanagedType.LPArray)] byte[] sb);

        [DllImport(DllName, EntryPoint = "Entities_GetVector4")]
        private static extern void GetVector4(IntPtr handle, int attrID, float[] @out);

        [DllImport(DllName, EntryPoint = "Entities_GetMatrix44")]
        private static extern void GetMatrix44(IntPtr handle, int attrID, float[] @out);

        #endregion

        #region Set attribute value

        [DllImport(DllName, EntryPoint = "Entities_SetBool")]
        private static extern void SetBool(IntPtr handle, int attrID, [MarshalAs(UnmanagedType.I1)] bool value);

        [DllImport(DllName, EntryPoint = "Entities_SetInt")]
        private static extern void SetInt(IntPtr handle, int attrID, int value);

        [DllImport(DllName, EntryPoint = "Entities_SetFloat")]
        private static extern void SetFloat(IntPtr handle, int attrID, float value);

        [DllImport(DllName, EntryPoint = "Entities_SetString")]
        private static extern void SetString(IntPtr handle, int attrID, [MarshalAs(UnmanagedType.LPArray)] byte[] value);

        [DllImport(DllName, EntryPoint = "Entities_SetStrID")]
        private static extern void SetStrID(IntPtr handle, int attrID, [MarshalAs(UnmanagedType.LPArray)] byte[] value);

        [DllImport(DllName, EntryPoint = "Entities_SetVector4")]
        private static extern void SetVector4(IntPtr handle, int attrID, float[] value);

        [DllImport(DllName, EntryPoint = "Entities_SetMatrix44")]
        private static extern void SetMatrix44(IntPtr handle, int attrID, float[] value);

        #endregion

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetUID")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool SetUID([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid, string newUid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_UpdateAttrs")]
        private static extern void UpdateAttrs([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);

        #endregion
    }
}
