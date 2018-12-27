using System;
using System.Runtime.InteropServices;

namespace CreatorIDE.EngineAPI
{
    public static class Transform
    {
        [DllImport(Engine.DllName, EntryPoint = "Transform_SetGroundConstraints")]
        public static extern void SetGroundConstraints([MarshalAs(UnmanagedType.I1)]bool NotAbove,
                                                       [MarshalAs(UnmanagedType.I1)]bool NotBelow);

        [DllImport(Engine.DllName, EntryPoint = "Transform_PlaceUnderMouse")]
        public static extern void PlaceUnderMouse();
    }
}
