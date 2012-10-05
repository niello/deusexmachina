using System.Runtime.InteropServices;

namespace CreatorIDE.Engine
{
    public static class EditorCamera
    {
        [DllImport(CideEngine.DllName, EntryPoint = "EditorCamera_SetFocusEntity")]
        public static extern void SetFocusEntity(string uid);
    }
}
