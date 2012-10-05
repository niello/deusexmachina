using System.Text;
using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    internal static class Entities
    {
        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetCount")]
        public static extern int GetCount();

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetUIDByIndex")]
        public static extern string GetUIDByIndex(int idx);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetNextFreeUIDOnLevel")]
        public static extern int GetNextFreeUIDOnLevel(string @base);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetCatByIndex")]
        public static extern string GetCategoryByIndex(int idx);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetCatByUID")]
        public static extern string GetCategoryByUID(string uid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetUIDUnderMouse")]
        public static extern string GetUIDUnderMouse();

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_CreateByCategory")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool Create([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid, string category);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_CreateFromTemplate")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool CreateFromTemplate([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid, string category, string tplID);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_BeginTemplate")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool BeginTemplate(string uid, string category, [MarshalAs(UnmanagedType.I1)]bool createIfNotExist);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_EndTemplate")]
        public static extern void EndTemplate();

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_AttachToWorld")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool AttachToWorld([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_DeleteByUID")]
        public static extern void Delete(string uid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_DeleteTemplate")]
        public static extern void DeleteTemplate(string uid, string category);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetCurrentByUID")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool SetCurrentEntity([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid);

        #region Get attribute value

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetBool")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool GetBool([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetInt")]
        public static extern int GetInt([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetFloat")]
        public static extern float GetFloat([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetString")]
        private static extern void _GetString([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, StringBuilder sb);
        public static string GetString(AppHandle handle, int attrID)
        {
            var sb = new StringBuilder(1024);
            _GetString(handle, attrID, sb);
            byte[] value = Encoding.Convert(Encoding.UTF8, Encoding.Default, Encoding.Default.GetBytes(sb.ToString()));
            return Encoding.Default.GetString(value);
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetStrID")]
        private static extern void _GetStrID([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, StringBuilder sb);
        public static string GetStrID(AppHandle handle, int attrID)
        {
            var sb = new StringBuilder(1024);
            try
            {
                _GetStrID(handle, attrID, sb);
            }
            catch
            {
                //??? Is it right?
            }
            byte[] value = Encoding.Convert(Encoding.UTF8, Encoding.Default, Encoding.Default.GetBytes(sb.ToString()));
            return Encoding.Default.GetString(value);
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetVector4")]
        private static extern void _GetVector4([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, float[] @out);
        public static Vector4 GetVector4(AppHandle handle, int attrID)
        {
            var data = new float[4];
            _GetVector4(handle, attrID, data);
            return new Vector4(data);
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_GetMatrix44")]
        private static extern void _GetMatrix44([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, float[] @out);
        public static Matrix44Ref GetMatrix44(AppHandle handle, int attrID)
        {
            var data = new float[16];
            _GetMatrix44(handle, attrID, data);
            return new Matrix44Ref(data);
        }

        #endregion

        #region Set attribute value

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetBool")]
        public static extern void SetBool([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, bool value);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetInt")]
        public static extern void SetInt([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, int value);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetFloat")]
        public static extern void SetFloat([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, float value);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetString")]
        private static extern void _SetString([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, string value);
        public static void SetString(AppHandle handle, int attrID, string value)
        {
            byte[] utfValue = Encoding.Convert(Encoding.Default, Encoding.UTF8, Encoding.Default.GetBytes(value));
            _SetString(handle, attrID, Encoding.Default.GetString(utfValue));
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetStrID")]
        private static extern void _SetStrID([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, string value);
        public static void SetStrID(AppHandle handle, int attrID, string value)
        {
            byte[] utfValue = Encoding.Convert(Encoding.Default, Encoding.UTF8, Encoding.Default.GetBytes(value));
            _SetStrID(handle, attrID, Encoding.Default.GetString(utfValue));
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetVector4")]
        private static extern void _SetVector4([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, int attrID, float[] value);
        public static void SetVector4(AppHandle handle, int attrID, Vector4 value)
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
        public static void SetMatrix44(AppHandle handle, int attrID, Matrix44Ref value)
        {
            float[] data = value.ToArray();
            _SetMatrix44(handle, attrID, data);
        }

        #endregion

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_SetUID")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool SetUID([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid, string newUid);

        [DllImport(CideEngine.DllName, EntryPoint = "Entities_UpdateAttrs")]
        public static extern void UpdateAttrs([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);
    }
}
