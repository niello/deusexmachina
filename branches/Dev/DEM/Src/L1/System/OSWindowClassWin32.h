#pragma once
#ifdef __WIN32__
#ifndef __DEM_L1_SYS_OS_WINDOW_CLASS_WIN32_H__
#define __DEM_L1_SYS_OS_WINDOW_CLASS_WIN32_H__

#include <Data/RefCounted.h>
#include <Data/String.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Win32 operating system window class implementation

namespace Sys
{

class COSWindowClassWin32: public Data::CRefCounted
{
protected:

	CString	Name;
	CString	IconName;

	HINSTANCE			hInst;
	ATOM				aWndClass;

	static LONG WINAPI	WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	COSWindowClassWin32();
	~COSWindowClassWin32() { Destroy(); }

	bool				Create(const char* pClassName, const char* pIconName = NULL);
	void				Destroy();

	const char*			GetName() const { return Name; }
	const char*			GetIconName() const { return IconName; }
	HINSTANCE			GetWin32HInstance() const { return hInst; }
	ATOM				GetWin32WindowClass() const { return aWndClass; }
};

typedef Ptr<COSWindowClassWin32> POSWindowClassWin32;

inline COSWindowClassWin32::COSWindowClassWin32():
	hInst(NULL),
	aWndClass(0)
{
	hInst = ::GetModuleHandle(NULL);
}
//---------------------------------------------------------------------

}

#endif
#endif //__WIN32__
