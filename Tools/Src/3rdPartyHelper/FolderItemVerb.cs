using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace ThirdPartyHelper
{
    [Guid("08EC3E00-50B0-11CF-960C-0080C7F4EE85")]
    [TypeLibType((short) 4160)]
    [ComImport]
    public interface FolderItemVerb
    {
        [DispId(1610743808)]
        object Application { [DispId(1610743808), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743809)]
        object Parent { [DispId(1610743809), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(0)]
        [IndexerName("Name")]
        string this[int i] { [DispId(0), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743811)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void DoIt();
    }
}
