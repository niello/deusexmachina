using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace ThirdPartyHelper
{
    [Guid("13709620-C279-11CE-A49E-444553540000")]
    [ClassInterface((short)0)]
    [TypeLibType((short)2)]
    [ComImport]
    public class ShellClass : Shell
    {
        [DispId(1610743808)]
        public extern virtual object Application { [DispId(1610743808), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743809)]
        public extern virtual object Parent { [DispId(1610743809), MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)] get; }

        [DispId(1610743810)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        public extern virtual Folder NameSpace([MarshalAs(UnmanagedType.Struct), In] object vDir);

        [DispId(1610743811)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.Interface)]
        public extern virtual Folder BrowseForFolder([In] int Hwnd, [MarshalAs(UnmanagedType.BStr), In] string Title, [In] int Options, [MarshalAs(UnmanagedType.Struct), In, Optional] object RootFolder);

        [DispId(1610743812)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        [return: MarshalAs(UnmanagedType.IDispatch)]
        public extern virtual object Windows();

        [DispId(1610743813)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void Open([MarshalAs(UnmanagedType.Struct), In] object vDir);

        [DispId(1610743814)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void Explore([MarshalAs(UnmanagedType.Struct), In] object vDir);

        [DispId(1610743815)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void MinimizeAll();

        [DispId(1610743816)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void UndoMinimizeALL();

        [DispId(1610743817)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void FileRun();

        [DispId(1610743818)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void CascadeWindows();

        [DispId(1610743819)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void TileVertically();

        [DispId(1610743820)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void TileHorizontally();

        [DispId(1610743821)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void ShutdownWindows();

        [DispId(1610743822)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void Suspend();

        [DispId(1610743823)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void EjectPC();

        [DispId(1610743824)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void SetTime();

        [DispId(1610743825)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void TrayProperties();

        [DispId(1610743826)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void Help();

        [DispId(1610743827)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void FindFiles();

        [DispId(1610743828)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void FindComputer();

        [DispId(1610743829)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void RefreshMenu();

        [DispId(1610743830)]
        [MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
        public extern virtual void ControlPanelItem([MarshalAs(UnmanagedType.BStr), In] string bstrDir);
    }
}
