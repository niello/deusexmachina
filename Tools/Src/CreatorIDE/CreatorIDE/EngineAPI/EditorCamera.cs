using System.Runtime.InteropServices;

namespace CreatorIDE.EngineAPI
{
    public static class EditorCamera
    {
        [DllImport(Engine.DllName, EntryPoint = "EditorCamera_SetFocusEntity")]
        public static extern void SetFocusEntity(string uid);
    }
}
