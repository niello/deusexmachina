using System;
using System.Runtime.InteropServices;

namespace CreatorIDE.EngineAPI
{
    public static class Transform
    {
        [DllImport(Engine.DllName, EntryPoint = "Transform_SetEnabled")]
        public static extern void SetEnabled([MarshalAs(UnmanagedType.I1)]bool enable);

        [DllImport(Engine.DllName, EntryPoint = "Transform_SetCurrentEntity")]
        public static extern void SetCurrentEntity(string uid);

        [DllImport(Engine.DllName, EntryPoint = "Transform_SetGroundRespectMode")]
        public static extern void SetGroundRespectMode([MarshalAs(UnmanagedType.I1)]bool limit,
                                                       [MarshalAs(UnmanagedType.I1)]bool snap);

        [DllImport(Engine.DllName, EntryPoint = "Transform_PlaceUnderMouse")]
        public static extern void PlaceUnderMouse();
    }
}
