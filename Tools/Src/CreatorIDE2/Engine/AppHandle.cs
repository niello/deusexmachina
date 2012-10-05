using System;
using System.Runtime.InteropServices;
using System.Threading;

namespace CreatorIDE.Engine
{
    [StructLayout(LayoutKind.Sequential)]
    internal struct AppHandle
    {
        public const UnmanagedType MarshalAs = UnmanagedType.LPStruct;

        public static readonly AppHandle Zero = new AppHandle(IntPtr.Zero);

        private IntPtr _handle;

        internal IntPtr Handle { get { return _handle; } }

        public AppHandle(IntPtr handle)
        {
            _handle = handle;
        }

        public bool Equals(AppHandle other)
        {
            return other._handle.Equals(_handle);
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (obj.GetType() != typeof (AppHandle)) return false;
            return Equals((AppHandle) obj);
        }

        public override int GetHashCode()
        {
            return _handle.GetHashCode();
        }

        public static bool operator ==(AppHandle left, AppHandle right)
        {
            return left.Equals(right);
        }

        public static bool operator !=(AppHandle left, AppHandle right)
        {
            return !left.Equals(right);
        }

        public static AppHandle InterlockedExchange(ref AppHandle handle, AppHandle value)
        {
            return new AppHandle(Interlocked.Exchange(ref handle._handle, value._handle));
        }
    }
}
