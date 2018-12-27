using System.Text;
using System.Runtime.InteropServices;

namespace CreatorIDE.EngineAPI
{
    public static class Entities
    {
        [DllImport(Engine.DllName, EntryPoint = "Entities_GetCount")]
        public static extern int GetCount();

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetUIDByIndex")]
        public static extern string GetUIDByIndex(int idx);

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetNextFreeUIDOnLevel")]
        public static extern int GetNextFreeUIDOnLevel(string @base);

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetCatByIndex")]
        public static extern string GetCategoryByIndex(int idx);

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetCatByUID")]
        public static extern string GetCategoryByUID(string uid);

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetUIDUnderMouse")]
        public static extern string GetUIDUnderMouse();

        [DllImport(Engine.DllName, EntryPoint = "Entities_CreateByCategory")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool Create(string uid, string category);

        [DllImport(Engine.DllName, EntryPoint = "Entities_CreateFromTemplate")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool CreateFromTemplate(string uid, string category, string tplID);

        [DllImport(Engine.DllName, EntryPoint = "Entities_BeginTemplate")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool BeginTemplate(string uid, string category, [MarshalAs(UnmanagedType.I1)]bool createIfNotExist);

        [DllImport(Engine.DllName, EntryPoint = "Entities_EndTemplate")]
        public static extern void EndTemplate();

        [DllImport(Engine.DllName, EntryPoint = "Entities_AttachToWorld")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool AttachToWorld();

        [DllImport(Engine.DllName, EntryPoint = "Entities_DeleteByUID")]
        public static extern void Delete(string uid);

        [DllImport(Engine.DllName, EntryPoint = "Entities_DeleteTemplate")]
        public static extern void DeleteTemplate(string uid, string category);

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetCurrentByUID")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool SetCurrentEntity(string uid);

        #region Get attribute value

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetBool")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool GetBool(int attrID);

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetInt")]
        public static extern int GetInt(int attrID);

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetFloat")]
        public static extern float GetFloat(int attrID);

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetString")]
        private static extern void _GetString(int attrID, StringBuilder sb);
        public static string GetString(int attrID)
        {
            var sb = new StringBuilder(1024);
            _GetString(attrID, sb);
            byte[] value = Encoding.Convert(Encoding.UTF8, Encoding.Default, Encoding.Default.GetBytes(sb.ToString()));
            return Encoding.Default.GetString(value);
        }

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetStrID")]
        private static extern void _GetStrID(int attrID, StringBuilder sb);
        public static string GetStrID(int attrID)
        {
            var sb = new StringBuilder(1024);
            try
            {
                _GetStrID(attrID, sb);
            }
            catch
            {
                //??? Is it right?
            }
            byte[] value = Encoding.Convert(Encoding.UTF8, Encoding.Default, Encoding.Default.GetBytes(sb.ToString()));
            return Encoding.Default.GetString(value);
        }

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetVector4")]
        private static extern void _GetVector4(int attrID, float[] @out);
        public static Vector4 GetVector4(int attrID)
        {
            var data = new float[4];
            _GetVector4(attrID, data);
            return new Vector4(data);
        }

        [DllImport(Engine.DllName, EntryPoint = "Entities_GetMatrix44")]
        private static extern void _GetMatrix44(int attrID, float[] @out);
        public static Matrix44Ref GetMatrix44(int attrID)
        {
            var data = new float[16];
            _GetMatrix44(attrID, data);
            return new Matrix44Ref(data);
        }

        #endregion

        #region Set attribute value

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetBool")]
        public static extern void SetBool(int attrID, bool value);

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetInt")]
        public static extern void SetInt(int attrID, int value);

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetFloat")]
        public static extern void SetFloat(int attrID, float value);

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetString")]
        private static extern void _SetString(int attrID, string value);
        public static void SetString(int attrID, string value)
        {
            byte[] utfValue = Encoding.Convert(Encoding.Default, Encoding.UTF8, Encoding.Default.GetBytes(value));
            _SetString(attrID, Encoding.Default.GetString(utfValue));
        }

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetStrID")]
        private static extern void _SetStrID(int attrID, string value);
        public static void SetStrID(int attrID, string value)
        {
            byte[] utfValue = Encoding.Convert(Encoding.Default, Encoding.UTF8, Encoding.Default.GetBytes(value));
            _SetStrID(attrID, Encoding.Default.GetString(utfValue));
        }

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetVector4")]
        private static extern void _SetVector4(int attrID, float[] value);
        public static void SetVector4(int attrID, Vector4 value)
        {
            float[] data = new float[4];
            data[0] = value.X;
            data[1] = value.Y;
            data[2] = value.Z;
            data[3] = value.W;
            _SetVector4(attrID, data);
        }

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetMatrix44")]
        private static extern void _SetMatrix44(int attrID, float[] value);
        public static void SetMatrix44(int attrID, Matrix44Ref value)
        {
            float[] data = value.ToArray();
            _SetMatrix44(attrID, data);
        }

        #endregion

        [DllImport(Engine.DllName, EntryPoint = "Entities_SetUID")]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool SetUID(string uid, string newUid);

        [DllImport(Engine.DllName, EntryPoint = "Entities_UpdateAttrs")]
        public static extern void UpdateAttrs();
    }
}
