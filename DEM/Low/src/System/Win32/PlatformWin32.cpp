#if DEM_PLATFORM_WIN32
#include <System/Win32/OSFileSystemWin32.h>
#include "PlatformWin32.h"
#include <System/Win32/OSWindowWin32.h>
#include <IO/PathUtils.h>
#include <shlobj.h>

namespace DEM { namespace Sys
{

CPlatformWin32::CPlatformWin32(HINSTANCE hInstance)
	: hInst(hInstance)
{
	LONGLONG PerfFreq;
	QueryPerformanceFrequency((LARGE_INTEGER*)&PerfFreq);
	PerfFreqMul = 1.0 / PerfFreq;

	FileSystemInterface.reset(n_new(COSFileSystemWin32));
}
//---------------------------------------------------------------------

CPlatformWin32::~CPlatformWin32()
{
	if (aGUIWndClass)
	{
		if (!UnregisterClass((const char*)aGUIWndClass, hInst))
			::Sys::Error("CPlatformWin32::~CPlatformWin32() > UnregisterClass() failed!\n");
		aGUIWndClass = 0;
	}
}
//---------------------------------------------------------------------

double CPlatformWin32::GetSystemTime() const
{
	LONGLONG PerfTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&PerfTime);
	return PerfTime * PerfFreqMul;
}
//---------------------------------------------------------------------

LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLong(hWnd, 0);
	LONG Result = 0;
	if (pWnd && pWnd->HandleWindowMessage(uMsg, wParam, lParam, Result)) return Result;
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

POSWindow CPlatformWin32::CreateGUIWindow()
{
	if (!aGUIWndClass)
	{
		WNDCLASSEX WndClass;
		memset(&WndClass, 0, sizeof(WndClass));
		WndClass.cbSize        = sizeof(WndClass);
		WndClass.style         = CS_DBLCLKS; // | CS_HREDRAW | CS_VREDRAW;
		WndClass.lpfnWndProc   = WindowProc;
		WndClass.cbClsExtra    = 0;
		WndClass.cbWndExtra    = sizeof(void*); // used to hold 'this' pointer
		WndClass.hInstance     = hInst;
		WndClass.hIcon         = ::LoadIcon(NULL, IDI_APPLICATION); // TODO: default DEM icon
		WndClass.hIconSm       = NULL; // set it too?
		WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		WndClass.lpszMenuName  = NULL;
		WndClass.lpszClassName = "DeusExMachina::MainWindow";
		aGUIWndClass = ::RegisterClassEx(&WndClass);
		
		if (!aGUIWndClass) return nullptr;
	}

	POSWindowWin32 Wnd = n_new(COSWindowWin32(hInst, aGUIWndClass));
	return Wnd->GetHWND() ? Wnd.Get() : nullptr;
}
//---------------------------------------------------------------------

void CPlatformWin32::Update()
{
	MSG Msg;
	while (::PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
	{
		// Process accelerators of our own windows
		if (aGUIWndClass && Msg.hwnd)
		{
			ATOM aClass = ::GetClassWord(Msg.hwnd, GCW_ATOM);
			if (aClass == aGUIWndClass)
			{
				// Only our windows store engine window pointer at 0
				COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLong(Msg.hwnd, 0);
				HACCEL hAccel = pWnd ? pWnd->GetWin32AcceleratorTable() : 0;
				if (hAccel && ::TranslateAccelerator(Msg.hwnd, hAccel, &Msg) != FALSE) continue;
			}
		}

		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);
	}
}
//---------------------------------------------------------------------

IOSFileSystem* CPlatformWin32::GetFileSystemInterface() const
{
	return FileSystemInterface.get();
}
//---------------------------------------------------------------------

bool CPlatformWin32::GetSystemFolderPath(ESystemFolder Code, CString& OutPath) const
{
	char pRawPath[DEM_MAX_PATH];
	if (Code == SysFolder_Temp)
	{
		if (!::GetTempPath(sizeof(pRawPath), pRawPath)) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}
	else if (Code == SysFolder_Bin || Code == SysFolder_Home)
	{
		if (!::GetModuleFileName(NULL, pRawPath, sizeof(pRawPath))) FAIL;
		CString PathToExe(pRawPath);
		PathToExe.Replace('\\', '/');
		OutPath = PathUtils::CollapseDots(PathUtils::ExtractDirName(PathToExe));
	}
	else if (Code == SysFolder_WorkingDir)
	{
		if (!::GetCurrentDirectory(sizeof(pRawPath), pRawPath)) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}
	else
	{
		int CSIDL;
		switch (Code)
		{
			case SysFolder_User:		CSIDL = CSIDL_MYDOCUMENTS | CSIDL_FLAG_CREATE; break;
			case SysFolder_AppData:		CSIDL = CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE; break;
			case SysFolder_Programs:	CSIDL = CSIDL_PROGRAM_FILES; break;
			default:					FAIL;
		}

		if (FAILED(::SHGetFolderPath(0, CSIDL, NULL, 0, pRawPath))) FAIL;
		OutPath = pRawPath;
		OutPath.Replace('\\', '/');
	}

	PathUtils::EnsurePathHasEndingDirSeparator(OutPath);

	OK;
}
//---------------------------------------------------------------------

}};

#endif
