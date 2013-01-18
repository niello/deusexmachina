using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace ThirdPartyHelper
{
    [Guid("D8F015C0-C278-11CE-A49E-444553540000")]
    [TypeLibType((short)4176)]
    [ComImport]
    public interface IShellDispatch
    {
        [DispId(1610743808)]
        object Application { [DispId(1610743808), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743809)]
        object Parent { [DispId(1610743809), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743810)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        Folder NameSpace([MarshalAs(UnmanagedType.Struct), In] object vDir);

        [DispId(1610743811)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        Folder BrowseForFolder([In] int Hwnd, [MarshalAs(UnmanagedType.BStr), In] string Title, [In] int Options, [MarshalAs(UnmanagedType.Struct), In, Optional] object RootFolder);

        [DispId(1610743812)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.IDispatch)]
        object Windows();

        [DispId(1610743813)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Open([MarshalAs(UnmanagedType.Struct), In] object vDir);

        [DispId(1610743814)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Explore([MarshalAs(UnmanagedType.Struct), In] object vDir);

        [DispId(1610743815)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void MinimizeAll();

        [DispId(1610743816)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void UndoMinimizeALL();

        [DispId(1610743817)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void FileRun();

        [DispId(1610743818)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void CascadeWindows();

        [DispId(1610743819)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void TileVertically();

        [DispId(1610743820)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void TileHorizontally();

        [DispId(1610743821)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void ShutdownWindows();

        [DispId(1610743822)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Suspend();

        [DispId(1610743823)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void EjectPC();

        [DispId(1610743824)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void SetTime();

        [DispId(1610743825)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void TrayProperties();

        [DispId(1610743826)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void Help();

        [DispId(1610743827)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void FindFiles();

        [DispId(1610743828)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void FindComputer();

        [DispId(1610743829)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void RefreshMenu();

        [DispId(1610743830)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        void ControlPanelItem([MarshalAs(UnmanagedType.BStr), In] string bstrDir);
    }
}
