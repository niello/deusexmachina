using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace ThirdPartyHelper
{
    [Guid("FAC32C80-CBE4-11CE-8350-444553540000")]
    [TypeLibType((short) 4160)]
    [ComImport]
    public interface FolderItem
    {
        [DispId(1610743808)]
        object Application { [DispId(1610743808), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743809)]
        object Parent { [DispId(1610743809), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(0)]
        [IndexerName("Name")]
        string this[int i] { [DispId(0), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; [DispId(0), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] set; }

        [DispId(1610743812)]
        string Path { [DispId(1610743812), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743813)]
        object GetLink { [DispId(1610743813), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743814)]
        object GetFolder { [DispId(1610743814), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743815)]
        bool IsLink { [DispId(1610743815), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743816)]
        bool IsFolder { [DispId(1610743816), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743817)]
        bool IsFileSystem { [DispId(1610743817), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743818)]
        bool IsBrowsable { [DispId(1610743818), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743819)]
        DateTime ModifyDate { [DispId(1610743819), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; [DispId(1610743819), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] set; }

        [DispId(1610743821)]
        int Size { [DispId(1610743821), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743822)]
        string Type { [DispId(1610743822), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743823)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        FolderItemVerbs Verbs();

        [DispId(1610743824)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void InvokeVerb([MarshalAs(UnmanagedType.Struct), In, Optional] object vVerb);
    }
}
