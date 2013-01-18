using System.Collections;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;


namespace ThirdPartyHelper
{
    [Guid("1F8352C0-50B0-11CF-960C-0080C7F4EE85")]
    [TypeLibType((short)4160)]
    [ComImport]
    public interface FolderItemVerbs : IEnumerable
    {
        [DispId(1610743808)]
        int Count { [DispId(1610743808), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743809)]
        object Application { [DispId(1610743809), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743810)]
        object Parent { [DispId(1610743810), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743811)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        FolderItemVerb Item([MarshalAs(UnmanagedType.Struct), In, Optional] object index);

        [DispId(-4)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.CustomMarshaler, MarshalType = "System.Runtime.InteropServices.CustomMarshalers.EnumeratorToEnumVariantMarshaler, CustomMarshalers, Version=2.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a")]
        IEnumerator GetEnumerator();
    }
}
