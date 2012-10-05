using System.Text;
using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    /// <summary>
    /// Game entity categories and templates
    /// </summary>
    internal static class Categories
    {
        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetCount")]
        public static extern int GetCount();

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetName")]
        public static extern string GetName(int idx);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_Delete")]
        public static extern void Delete(string name);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetInstAttrCount")]
        public static extern int GetInstAttrCount(int idx);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetTplAttrCount")]
        public static extern int GetTplAttrCount(int idx);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetAttrID")]
        private static extern string _GetAttrID(int catIdx,
                                                int attrIdx,
                                                ref int type,
                                                ref int attrId,
                                                [MarshalAs(UnmanagedType.I1)]ref bool isReadWrite);
        public static AttrID GetAttrID(int catIdx, int attrIdx)
        {
            int typeID = 0, id = 0;
            bool isReadWrite = false;
            var attrName = _GetAttrID(catIdx, attrIdx, ref typeID, ref id, ref isReadWrite);
            return new AttrID(id, attrName, (EDataType) typeID, isReadWrite);
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetAttrIDByName")]
        private static extern string _GetAttrID(string name,
                                                ref int type,
                                                ref int attrID,
                                                [MarshalAs(UnmanagedType.I1)]ref bool isReadWrite);
        public static AttrID GetAttrID(string name)
        {
            int typeID = 0, id=0;
            bool isReadWrite = false;
            var attrName = _GetAttrID(name, ref typeID, ref id, ref isReadWrite);
            return new AttrID(id, attrName, (EDataType) typeID, isReadWrite);
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_ParseAttrDescs")]
        public static extern bool ParseAttrDescs([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string fileName);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetAttrDescCount")]
        public static extern int GetAttrDescCount([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);

        //!!!need C# HRD reader!
        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetAttrDesc")]
        private static extern string _GetAttrDesc([MarshalAs(AppHandle.MarshalAs)] AppHandle handle,
                                                  int idx,
                                                  StringBuilder cat,
                                                  StringBuilder desc,
                                                  StringBuilder resourceFilter,
                                                  [MarshalAs(UnmanagedType.I1)]ref bool isReadOnly,
                                                  [MarshalAs(UnmanagedType.I1)]ref bool showInList,
                                                  [MarshalAs(UnmanagedType.I1)]ref bool instanceOnly);
        public static string GetAttrDesc(AppHandle handle, int idx, out AttrDesc desc)
        {
            desc = new AttrDesc();
            StringBuilder sbCat = new StringBuilder(256);
            StringBuilder sbDesc = new StringBuilder(1024);
            StringBuilder sbResFilter = new StringBuilder(1024);

            bool isReadOnly = false, showInList = false, instanceOnly = false;
            string name = _GetAttrDesc(handle, idx, sbCat, sbDesc, sbResFilter,
                ref isReadOnly, ref showInList, ref instanceOnly);

            var resFilter = sbResFilter.ToString().Trim().ToLower().Split(';');
            if (resFilter.Length > 1)
            {
                desc.ResourceDir = resFilter[0];
                desc.ResourceExt = resFilter[1];
            }
            string category = sbCat.ToString().Trim();
            string description = sbDesc.ToString().Trim();
            if (category.Length > 0) desc.Category = category;
            if (description.Length > 0) desc.Description = description;
            return name;
        }

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetTemplateCount")]
        public static extern int GetTemplateCount(string catName);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetTemplateID")]
        public static extern string GetTemplateID(string catName, int idx);

        [DllImport(CideEngine.DllName)]
        private static extern void Categories_BeginCreate(string name, string cppClass, string tplTable, string instTable);
        [DllImport(CideEngine.DllName)]
        private static extern void Categories_AddProperty(string prop);
        [DllImport(CideEngine.DllName)]
        private static extern int Categories_EndCreate();
        public static int CreateNew(string name, string cppClass, string tplTable, string instTable, string[] props)
        {
            Categories_BeginCreate(name, cppClass, tplTable, instTable);
            foreach (string prop in props) Categories_AddProperty(prop);
            return Categories_EndCreate();
        }
    }
}
