using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    internal static class Transform
    {
        [DllImport(CideEngine.DllName, EntryPoint = "Transform_SetEnabled")]
        public static extern void SetEnabled([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, [MarshalAs(UnmanagedType.I1)]bool enable);

        [DllImport(CideEngine.DllName, EntryPoint = "Transform_SetCurrentEntity")]
        public static extern void SetCurrentEntity([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, string uid);

        [DllImport(CideEngine.DllName, EntryPoint = "Transform_SetGroundRespectMode")]
        public static extern void SetGroundRespectMode([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, 
                                                       [MarshalAs(UnmanagedType.I1)]bool limit,
                                                       [MarshalAs(UnmanagedType.I1)]bool snap);

        [DllImport(CideEngine.DllName, EntryPoint = "Transform_PlaceUnderMouse")]
        public static extern void PlaceUnderMouse([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);
    }
}
