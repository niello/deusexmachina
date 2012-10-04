using System;
using System.Runtime.InteropServices;
using System.Text;

namespace CreatorIDE.EngineAPI
{
    public static class Engine
    {
#if (DEBUG)
        public const string DllName = "EngineAPI_d.dll";
#else
	    public const string DllName = "EngineAPI.dll";
#endif
        
        [DllImport(DllName)]
        public static extern int Init(IntPtr parentHwnd, string projDir);

        [DllImport(DllName)]
        public static extern void Release();

        [DllImport(DllName)]
        [return: MarshalAs(UnmanagedType.I1)]
        public static extern bool Advance();

        [DllImport(DllName, EntryPoint = "GetDLLName")]
        private static extern void _GetDllName(StringBuilder name);
        public static string GetDllName()
        {
            StringBuilder sb = new StringBuilder(256);
            _GetDllName(sb);
            return sb.ToString();
        }

        [DllImport(DllName, EntryPoint = "GetDLLVersion")]
        private static extern void _GetDllVersion(StringBuilder version);
        public static string GetDllVersion()
        {
            StringBuilder sb = new StringBuilder(256);
            _GetDllVersion(sb);
            return sb.ToString();
        }

        [DllImport(DllName)]
        public static extern int GetDLLVersionCode();

        [DllImport(DllName)]
        public static extern void SetMouseButtonCallback(MouseButtonCallback callback);
    }

    public enum EMouseAction
    {
        Down = 0,
        Click,
        Up,
        DoubleClick
    }

    public delegate void MouseButtonCallback(int x, int y, int button, EMouseAction action);
}
