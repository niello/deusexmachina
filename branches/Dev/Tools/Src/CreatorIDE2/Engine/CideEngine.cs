using System;
using System.Runtime.InteropServices;
using System.Text;

namespace CreatorIDE.Engine
{
    public sealed partial class CideEngine: IDisposable
    {
        private delegate void MouseButtonCallback(int x, int y, int button, EMouseAction action);

        private delegate bool DataPathCallback(string dataPath, out IntPtr mangledPath);

        private delegate void ReleaseMemoryCallback(IntPtr ptr);

#if (DEBUG)
        internal const string DllName = "EngineAPI2_d.dll";
#else
	    internal const string DllName = "EngineAPI2.dll";
#endif

        private readonly MouseButtonCallback _mouseButtonCalback;
        private readonly DataPathCallback _dataPathCallback;
        private readonly ReleaseMemoryCallback _releaseMemoryCallback;

        private bool _isInitialized;

        private AppHandle _engineHandle;

        public event EventHandler<EnginePathRequestEventArgs> PathRequest;

        public event EventHandler<EngineMouseClickEventArgs> MouseClick;

        public bool IsInitialized { get { return _isInitialized; } }

        public CideEngine()
        {
            // Keeping callbacks in the CideEngine to prevent garbage collection
            _mouseButtonCalback = OnMouseButtonCallback;
            _dataPathCallback = OnDataPathCallback;
            _releaseMemoryCallback = FreeHGlobal;

            _engineHandle = new AppHandle(CreateEngine());
        }

        public void Init(IntPtr parentHwnd, string projDir)
        {
            SetDataPathCallback(_engineHandle.Handle, _dataPathCallback, _releaseMemoryCallback);
            SetMouseButtonCallback(_engineHandle.Handle, _mouseButtonCalback);

            int code = Init(_engineHandle.Handle, parentHwnd, projDir);
            if (code != 0)
                throw new EngineInitializationException(string.Format(SR.GetString(SR.EngineInitFailFormat), code));

            _isInitialized = true;
        }

        private void OnMouseButtonCallback(int x, int y, int button, EMouseAction action)
        {
            var h = MouseClick;
            if (h == null)
                return;

            var args = new EngineMouseClickEventArgs(x, y, button, action);
            h(this, args);
        }

        private bool OnDataPathCallback(string dataPath, out IntPtr mangledPath)
        {
            var h = PathRequest;
            if (h != null)
            {
                var args = new EnginePathRequestEventArgs(dataPath);
                h(this, args);

                if (args.Handled)
                {
                    var path = args.NormalizedPath;
                    if (path != null)
                        path = path.Trim();
                    if (!string.IsNullOrEmpty(path))
                    {
                        mangledPath = Marshal.StringToHGlobalAnsi(path);
                        return true;
                    }
                }
            }

            mangledPath = IntPtr.Zero;
            return false;
        }

        public void LoadLevel(string levelID)
        {
            LoadLevel(_engineHandle.Handle, levelID);
        }

        public bool Advance()
        {
            return Advance(_engineHandle.Handle);
        }

        public int GetLevelCount()
        {
            return GetLevelCount(_engineHandle.Handle);
        }

        public LevelRecord GetLevelRecord(int index)
        {
            StringBuilder sb = new StringBuilder(256),
                          sb2 = new StringBuilder(256);

            GetLevelID(_engineHandle.Handle, index, sb, sb2);
            return new LevelRecord(sb.ToString(), sb2.ToString());
        }

        public void SetFocusEntity(string uid)
        {
            SetFocusEntity(_engineHandle.Handle, uid);
        }

        private static void FreeHGlobal(IntPtr hGlobal)
        {
            Marshal.FreeHGlobal(hGlobal);
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
        private static extern bool Advance(IntPtr handle);

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
        private static extern void SetDataPathCallback(IntPtr handle, DataPathCallback callback, ReleaseMemoryCallback releaseMemory);

        [DllImport(DllName, EntryPoint = "EditorCamera_SetFocusEntity")]
        private static extern void SetFocusEntity(IntPtr handle, [MarshalAs(UnmanagedType.LPStr)] string uid);

        #endregion

        #region DLL Import - Levels

        [DllImport(DllName, EntryPoint = "Levels_GetCount")]
        private static extern int GetLevelCount(IntPtr handle);

        [DllImport(DllName, EntryPoint = "Levels_GetIDName")]
        private static extern void GetLevelID(IntPtr handle, int idx, StringBuilder id, StringBuilder name);

        [DllImport(DllName, EntryPoint = "Levels_LoadLevel")]
        private static extern void LoadLevel(IntPtr handle, string levelID);

        [DllImport(CideEngine.DllName, EntryPoint = "Levels_CreateNew")]
        [return: MarshalAs(UnmanagedType.I1)]
        private static extern bool CreateNew(string id, string name, float[] center, float[] extents,
                                            string navMesh);

        [DllImport(CideEngine.DllName, EntryPoint = "Levels_RestoreDB")]
        private static extern void RestoreDB([MarshalAs(AppHandle.MarshalAs)] int handle, string startLevelID);

        [DllImport(CideEngine.DllName, EntryPoint = "Levels_SaveDB")]
        private static extern void SaveDB();

        #endregion
    }

    public enum EMouseAction
    {
        Down = 0,
        Click,
        Up,
        DoubleClick
    }
}
