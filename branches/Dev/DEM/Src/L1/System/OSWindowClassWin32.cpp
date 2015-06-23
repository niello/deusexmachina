#ifdef __WIN32__

#include "OSWindowClassWin32.h"

#include <System/OSWindowWin32.h>

namespace Sys
{

LONG WINAPI COSWindowClassWin32::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	COSWindowWin32* pWnd = (COSWindowWin32*)::GetWindowLong(hWnd, 0);
	LONG Result = 0;
	if (pWnd && pWnd->HandleWindowMessage(uMsg, wParam, lParam, Result)) return Result;
	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

bool COSWindowClassWin32::Create(const char* pClassName, const char* pIconName)
{
	if (aWndClass) FAIL;

	Name = pClassName;
	IconName = pIconName;

	HICON hIcon = NULL;
	if (IconName.IsValid()) hIcon = ::LoadIcon(hInst, IconName.CStr());
	if (!hIcon) hIcon = ::LoadIcon(NULL, IDI_APPLICATION);

	WNDCLASSEX WndClass;
	memset(&WndClass, 0, sizeof(WndClass));
	WndClass.cbSize        = sizeof(WndClass);
	WndClass.style         = CS_DBLCLKS;
	WndClass.lpfnWndProc   = WinProc;
	WndClass.cbClsExtra    = 0;
	WndClass.cbWndExtra    = sizeof(void*); // used to hold 'this' pointer
	WndClass.hInstance     = hInst;
	WndClass.hIcon         = hIcon;
	WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	WndClass.lpszMenuName  = NULL;
	WndClass.lpszClassName = Name.CStr();
	WndClass.hIconSm       = NULL;
	aWndClass = ::RegisterClassEx(&WndClass);
	return !!aWndClass;
}
//---------------------------------------------------------------------

void COSWindowClassWin32::Destroy()
{
	if (aWndClass)
	{
		if (!UnregisterClass((LPCSTR)aWndClass, hInst))
			Sys::Error("COSWindowClassWin32::Destroy(): UnregisterClass() failed!\n");
		aWndClass = 0;
	}
}
//---------------------------------------------------------------------

}

#endif //__WIN32__
