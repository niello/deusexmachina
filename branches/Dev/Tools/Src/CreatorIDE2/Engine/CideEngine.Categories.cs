using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    /// <summary>
    /// Game entity categories and templates
    /// </summary>
    partial class CideEngine
    {
        public Dictionary<string, AttrDesc> _attrCache;

        public int GetCategoryCount()
        {
            return GetCategoryCount(_engineHandle.Handle);
        }

        public int GetCategoryInstantAttrCount(int categoryIdx)
        {
            return GetCategoryInstantAttrCount(_engineHandle.Handle, categoryIdx);
        }

        public string GetCategoryName(int categoryIdx)
        {
            var name = new StringBuilder(256);
            GetCategoryName(_engineHandle.Handle, categoryIdx, name);
            return name.ToString();
        }

        public AttrID GetAttrID(int categoryIdx, int attrIdx)
        {
            int typeID, id;
            bool isReadWrite;
            var attrName = new StringBuilder(256);
            GetCategoryAttrID(_engineHandle.Handle, categoryIdx, attrIdx, out typeID, out id, out isReadWrite, attrName);
            return new AttrID(id, attrName.ToString(), (EDataType) typeID, isReadWrite);
        }

        public AttrDesc GetAttrDesc(string name)
        {
            if(_attrCache==null)
            {
                int count = GetAttrDescCount(_engineHandle.Handle);
                _attrCache = new Dictionary<string, AttrDesc>(count);
                for(int i=0; i<count; i++)
                {
                    AttrDesc attrDesc;
                    var attrName = GetAttrDesc(i, out attrDesc);
                    _attrCache.Add(attrName, attrDesc);
                }
            }

            AttrDesc result;
            return _attrCache.TryGetValue(name, out result) ? result : null;
        }

        private string GetAttrDesc(int idx, out AttrDesc desc)
        {
            desc = new AttrDesc();
            var sbCat = new StringBuilder(256);
            var sbDesc = new StringBuilder(1024);
            var sbResFilter = new StringBuilder(1024);

            bool isReadOnly, showInList, instanceOnly;
            string name = GetAttrDesc(_engineHandle.Handle, idx, sbCat, sbDesc, sbResFilter,
                out isReadOnly, out showInList, out instanceOnly);

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

        public Category GetCategory(int categoryIdx)
        {
            return new Category(this, categoryIdx);
        }

        #region DLL Import

        [DllImport(DllName, EntryPoint = "Categories_GetCount")]
        private static extern int GetCategoryCount(IntPtr handle);

        [DllImport(DllName, EntryPoint = "Categories_GetName")]
        private static extern void GetCategoryName(IntPtr handle,
                                                   int idx,
                                                   [MarshalAs(UnmanagedType.LPStr)] StringBuilder name);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_Delete")]
        private static extern void Delete(string name);

        [DllImport(DllName, EntryPoint = "Categories_GetInstAttrCount")]
        private static extern int GetCategoryInstantAttrCount(IntPtr handle, int idx);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetTplAttrCount")]
        private static extern int GetTplAttrCount(int idx);

        [DllImport(DllName, EntryPoint = "Categories_GetAttrID")]
        private static extern void GetCategoryAttrID(
            IntPtr handle,
            int catIdx,
            int attrIdx,
            out int type,
            out int attrId,
            [MarshalAs(UnmanagedType.I1)] out bool isReadWrite,
            [MarshalAs(UnmanagedType.LPStr)] StringBuilder attrName);

        [DllImport(DllName, EntryPoint = "Categories_GetAttrIDByName")]
        private static extern void GetCategoryAttrID(
            IntPtr handle,
            [MarshalAs(UnmanagedType.LPStr)] string name,
            out int type,
            out int attrID,
            [MarshalAs(UnmanagedType.I1)] out bool isReadWrite,
            [MarshalAs(UnmanagedType.LPStr)] StringBuilder attrName);
        //public static AttrID GetAttrID(int categoryIdx, int attrIdx)
        //{
        //    int typeID = 0, id = 0;
        //    bool isReadWrite = false;
        //    var attrName = _GetAttrID(name, ref typeID, ref id, ref isReadWrite);
        //    return new AttrID(id, attrName, (EDataType)typeID, isReadWrite);
        //}
        
        [DllImport(CideEngine.DllName, EntryPoint = "Categories_ParseAttrDescs")]
        private static extern bool ParseAttrDescs([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string fileName);

        [DllImport(DllName, EntryPoint = "Categories_GetAttrDescCount")]
        private static extern int GetAttrDescCount(IntPtr handle);

        //!!!need C# HRD reader!
        [DllImport(DllName, EntryPoint = "Categories_GetAttrDesc")]
        private static extern string GetAttrDesc(
            IntPtr handle,
            int idx,
            [MarshalAs(UnmanagedType.LPStr)]StringBuilder cat,
            [MarshalAs(UnmanagedType.LPStr)]StringBuilder desc,
            [MarshalAs(UnmanagedType.LPStr)]StringBuilder resourceFilter,
            [MarshalAs(UnmanagedType.I1)]out bool isReadOnly,
            [MarshalAs(UnmanagedType.I1)]out bool showInList,
            [MarshalAs(UnmanagedType.I1)]out bool instanceOnly);
        
        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetTemplateCount")]
        private static extern int GetTemplateCount(string catName);

        [DllImport(CideEngine.DllName, EntryPoint = "Categories_GetTemplateID")]
        private static extern string GetTemplateID(string catName, int idx);

        [DllImport(CideEngine.DllName)]
        private static extern void Categories_BeginCreate(string name, string cppClass, string tplTable, string instTable);
        [DllImport(CideEngine.DllName)]
        private static extern void Categories_AddProperty(string prop);
        [DllImport(CideEngine.DllName)]
        private static extern int Categories_EndCreate();
        //public static int CreateNew(string name, string cppClass, string tplTable, string instTable, string[] props)
        //{
        //    Categories_BeginCreate(name, cppClass, tplTable, instTable);
        //    foreach (string prop in props) Categories_AddProperty(prop);
        //    return Categories_EndCreate();
        //}

        #endregion
    }
}
