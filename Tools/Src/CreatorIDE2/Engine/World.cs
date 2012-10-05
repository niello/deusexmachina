using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    public static class World
    {
        [DllImport(CideEngine.DllName, EntryPoint = "World_TogglePause")]
        public static extern void TogglePause();

        [DllImport(CideEngine.DllName, EntryPoint = "World_SetPause")]
        public static extern void SetPause([MarshalAs(UnmanagedType.I1)]bool pause);
    }
}
