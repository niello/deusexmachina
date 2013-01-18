using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;


namespace ThirdPartyHelper
{
    [TypeLibType((short) 4160)]
    [Guid("BBCBDE60-C3FF-11CE-8350-444553540000")]
    [ComImport]
    public interface Folder
    {
        [DispId(0)]
        [IndexerName("Title")]
        string this[int i] { [DispId(0), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743809)]
        object Application { [DispId(1610743809), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743810)]
        object Parent { [DispId(1610743810), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743811)]
        Folder ParentFolder { [DispId(1610743811), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743812)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        FolderItems Items();

        [DispId(1610743813)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        FolderItem ParseName([MarshalAs(UnmanagedType.BStr), In] string bName);

        [DispId(1610743814)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void NewFolder([MarshalAs(UnmanagedType.BStr), In] string bName,
                       [MarshalAs(UnmanagedType.Struct), In, Optional] object vOptions);

        [DispId(1610743815)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void MoveHere([MarshalAs(UnmanagedType.Struct), In] object vItem,
                      [MarshalAs(UnmanagedType.Struct), In, Optional] object vOptions);

        [DispId(1610743816)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CopyHere([MarshalAs(UnmanagedType.Struct), In] object vItem,
                      [MarshalAs(UnmanagedType.Struct), In, Optional] object vOptions);

        [DispId(1610743817)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.BStr)]
        string GetDetailsOf([MarshalAs(UnmanagedType.Struct), In] object vItem, [In] int iColumn);
    }
}
