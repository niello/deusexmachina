using System;
using System.Text;
using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    partial class CideEngine
    {
        public int GetEntityCount()
        {
            return GetEntityCount(_engineHandle.Handle);
        }

        public string GetEntityCategory(int entityIdx)
        {
            var category = new StringBuilder(256);
            GetEntityCategory(_engineHandle.Handle, entityIdx, category);
            return category.ToString();
        }

        public string GetEntityUID(int entityIndex)
        {
            var uid = new StringBuilder(256);
            GetEntityUID(_engineHandle.Handle, entityIndex, uid);
            return uid.ToString();
        }

        public bool SetCurrentEntity(string entityUID)
        {
            return SetCurrentEntity(_engineHandle.Handle, entityUID);
        }

        public bool GetBool(int attrID)
        {
            return GetBool(_engineHandle.Handle, attrID);
        }

        public float GetFloat(int attrID)
        {
            return GetFloat(_engineHandle.Handle, attrID);
        }

        public int GetInt(int attrID)
        {
            return GetInt(_engineHandle.Handle, attrID);
        }

        public Matrix44Ref GetMatrix44(int attrID)
        {
            var data = new float[16];
            GetMatrix44(_engineHandle.Handle, attrID, data);
            return new Matrix44Ref(data);
        }

        public string GetStrID(int attrID)
        {
            var buffer = new byte[256];
            GetStrID(_engineHandle.Handle, attrID, buffer);
            var str = GetUtf8String(buffer);
            return str;
        }

        public string GetString(int attrID)
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

        public Vector4 GetVector4(int attrID)
        {
            var data = new float[4];
            GetVector4(_engineHandle.Handle, attrID, data);
            return new Vector4(data);
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

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetBool")]
        private static extern void SetBool([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, bool value);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetInt")]
        private static extern void SetInt([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, int value);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetFloat")]
        private static extern void SetFloat([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, float value);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetString")]
        private static extern void _SetString([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, string value);
        private static void SetString(AppHandle handle, int attrID, string value)
        {
            byte[] utfValue = Encoding.Convert(Encoding.Default, Encoding.UTF8, Encoding.Default.GetBytes(value));
            _SetString(handle, attrID, Encoding.Default.GetString(utfValue));
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetStrID")]
        private static extern void _SetStrID([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, string value);
        private static void SetStrID(AppHandle handle, int attrID, string value)
        {
            byte[] utfValue = Encoding.Convert(Encoding.Default, Encoding.UTF8, Encoding.Default.GetBytes(value));
            _SetStrID(handle, attrID, Encoding.Default.GetString(utfValue));
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetVector4")]
        private static extern void _SetVector4([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, float[] value);
        private static void SetVector4(AppHandle handle, int attrID, Vector4 value)
        {
            float[] data = new float[4];
            data[0] = value.X;
            data[1] = value.Y;
            data[2] = value.Z;
            data[3] = value.W;
            _SetVector4(handle, attrID, data);
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetMatrix44")]
        private static extern void _SetMatrix44([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, float[] value);
        private static void SetMatrix44(AppHandle handle, int attrID, Matrix44Ref value)
        {
            float[] data = value.ToArray();
            _SetMatrix44(handle, attrID, data);
        }

        #endregion

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetUID")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool SetUID([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid, string newUid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_UpdateAttrs")]
        private static extern void UpdateAttrs([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);

        #endregion
    }
}
