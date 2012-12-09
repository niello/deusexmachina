using System;
using System.Runtime.InteropServices;
using System.Text;

namespace CreatorIDE.Engine
{
    public sealed class CideEngine:IDisposable
    {

#if (DEBUG)
        public const string DllName = "EngineAPI2_d.dll";
#else
	    public const string DllName = "EngineAPI2.dll";
#endif

        private AppHandle _engineHandle;

        public event EventHandler<EnginePathRequestEventArgs> PathRequest;

        public event EventHandler<EngineMouseClickEventArgs> MouseClick;

        public CideEngine()
        {
            _engineHandle = new AppHandle(CreateEngine());
        }

        public int Init(IntPtr parentHwnd, string projDir)
        {
            SetDataPathCallback(_engineHandle.Handle, OnDataPathCallback);
            SetMouseButtonCallback(_engineHandle.Handle, OnMouseButtonCallback);

            return Init(_engineHandle.Handle, parentHwnd, projDir);
        }

        private void OnMouseButtonCallback(int x, int y, int button, EMouseAction action)
        {
            var h = MouseClick;
            if (h == null)
                return;

            var args = new EngineMouseClickEventArgs(x, y, button, action);
            h(this, args);
        }

        private bool OnDataPathCallback(string dataPath, out string mangledPath)
        {
            var h = PathRequest;
            if (h != null)
            {
                var args = new EnginePathRequestEventArgs(dataPath);
                h(this, args);

                if(args.Handled)
                {
                    var path = args.NormalizedPath;
                    if (path != null)
                        path = path.Trim();
                    if(!string.IsNullOrEmpty(path))
                    {
                        mangledPath = path;
                        return true;
                    }
                }
            }

            mangledPath = null;
            return false;
        }


        public void Dispose()
        {
            var handle = AppHandle.InterlockedExchange(ref _engineHandle, AppHandle.Zero);
            if (handle == AppHandle.Zero)
                return;
            
            Release(handle.Handle);
        }

        ~CideEngine()
        {
            Dispose();
        }

        #region DLL Import

        [DllImport(DllName)]
        private static extern int Init(IntPtr handle, IntPtr parentHwnd, string projDir);

        [DllImport(DllName)]
        private static extern void Release(IntPtr handle);

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
        private static extern void SetMouseButtonCallback(IntPtr handle, MouseButtonCallback callback);

        [DllImport(DllName)]
        private static extern IntPtr CreateEngine();

        [DllImport(DllName)]
        private static extern void SetDataPathCallback(IntPtr handle, DataPathCallback callback);

        #endregion
    }

    public enum EMouseAction
    {
        Down = 0,
        Click,
        Up,
        DoubleClick
    }

    internal delegate void MouseButtonCallback(int x, int y, int button, EMouseAction action);

    internal delegate bool DataPathCallback(string dataPath, out string mangledPath);
}
