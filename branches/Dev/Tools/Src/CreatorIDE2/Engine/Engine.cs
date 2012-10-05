using System;
using System.Runtime.InteropServices;
using System.Text;

namespace CreatorIDE.Engine
{
    public sealed class CideEngine:IDisposable
    {
        [Obsolete("Temporary")]
        //public static readonly CideEngine Instance = new CideEngine();

#if (DEBUG)
        public const string DllName = "EngineAPI_d.dll";
#else
	    public const string DllName = "EngineAPI.dll";
#endif

        private AppHandle _engineHandle;

        public CideEngine()
        {
            _engineHandle = new AppHandle(CreateEngine());
        }

        public int Init(IntPtr parentHwnd, string projDir)
        {
            return Init(_engineHandle.Handle, parentHwnd, projDir);
        }

        public void Dispose()
        {
            var handle = AppHandle.InterlockedExchange(ref _engineHandle, AppHandle.Zero);
            if (handle == AppHandle.Zero)
                return;
            
            Release(handle);
        }

        ~CideEngine()
        {
            Dispose();
        }

        #region DLL Import

        [DllImport(DllName)]
        private static extern int Init(IntPtr handle, IntPtr parentHwnd, string projDir);

        [DllImport(DllName)]
        private static extern void Release([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);

        [DllImport(DllName)]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool Advance([MarshalAs(AppHandle.MarshalAs)] AppHandle handle);

        [DllImport(DllName, EntryPoint = "GetDLLName")]
        private static extern void _GetDllName([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, StringBuilder name);
        public string GetDllName()
        {
            var sb = new StringBuilder(256);
            _GetDllName(_engineHandle, sb);
            return sb.ToString();
        }

        [DllImport(DllName, EntryPoint = "GetDLLVersion")]
        private static extern void _GetDllVersion([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, StringBuilder version);
        private static string GetDllVersion(AppHandle handle)
        {
            StringBuilder sb = new StringBuilder(256);
            _GetDllVersion(handle, sb);
            return sb.ToString();
        }

        [DllImport(DllName)]
        private static extern int GetDLLVersionCode();

        [DllImport(DllName)]
        private static extern void SetMouseButtonCallback([MarshalAs(AppHandle.MarshalAs)] AppHandle handle, MouseButtonCallback callback);

        [DllImport(DllName)]
        private static extern IntPtr CreateEngine();

        #endregion
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
