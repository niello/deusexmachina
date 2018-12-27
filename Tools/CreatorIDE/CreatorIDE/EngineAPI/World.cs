using System.Runtime.InteropServices;

namespace CreatorIDE.EngineAPI
{
    public static class World
    {
        [DllImport(Engine.DllName, EntryPoint = "World_TogglePause")]
        public static extern void TogglePause();

        [DllImport(Engine.DllName, EntryPoint = "World_SetPause")]
        public static extern void SetPause([MarshalAs(UnmanagedType.I1)]bool pause);
    }
}
