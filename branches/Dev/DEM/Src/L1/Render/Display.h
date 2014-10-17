#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_H__
#define __DEM_L1_RENDER_DISPLAY_H__
//#ifdef __WIN32__

#include <Data/String.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//!!!OS-based, so can avoid virtuality and use define-based approach as in N3!

// Display handles a viewport (in the Win32 it is a window of the host OS)
// and a display mode that is used by the render device.

namespace Render
{

class CDisplay
{
protected:

	//???!!!use CSimpleString?!
	CString			WindowTitle;
	CString			IconName;
	bool			IsWndOpen;
	bool			IsWndMinimized;
	bool			AlwaysOnTop;

	HINSTANCE		hInst;
	HWND			hWnd;
	HWND			hWndParent;
	HACCEL			hAccel;
	ATOM			aWndClass;
	LONG			StyleWindowed;		///< WS_* flags for windowed mode
	LONG			StyleFullscreen;	///< WS_* flags for full-screen mode
	LONG			StyleChild;			///< WS_* flags for child mode

	bool				HandleWindowMessage(HWND _hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG Result);
	void				CalcWindowRect(int& X, int& Y, int& W, int& H);

	static LONG WINAPI	WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	CDisplay();
	~CDisplay();

	//!!!rename class to Window and remove this word from method names!
	bool				OpenWindow();
	void				CloseWindow();
	void				ProcessWindowMessages();
	void				ResetWindow();
	void				RestoreWindow();
	void				MinimizeWindow();

	void				SetWindowTitle(const char* pTitle);
	const CString&		GetWindowTitle() const { return WindowTitle; }
	void				SetWindowIcon(const char* pIconName);
	const CString&		GetWindowIcon() const { return IconName; }

	//???!!!SetSize?! store in members, use in GetAbsoluteXY/GetRelativeXY?!

	// Based on real window size
	bool				GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const;
	bool				GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	bool				IsWindowOpen() const { return IsWndOpen; }
	bool				IsWindowMinimized() const { return IsWndMinimized; }
	HWND				GetAppHwnd() const { return hWnd; }
	void				SetParentWindow(HWND Parent) { hWndParent = Parent; }
	HWND				GetParentWindow() const { return hWndParent; }
	ATOM				GetWndClass() const { return aWndClass; }
};

inline bool CDisplay::GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const
{
	RECT r;
	if (!hWnd || !GetClientRect(hWnd, &r)) FAIL;
	XAbs = (int)(XRel * n_max(r.right - r.left, 1));
	YAbs = (int)(YRel * n_max(r.bottom - r.top, 1));
	OK;
}
//---------------------------------------------------------------------

inline bool CDisplay::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	RECT r;
	if (!hWnd || !GetClientRect(hWnd, &r)) FAIL;
	XRel = XAbs / float(n_max(r.right - r.left, 1));
	YRel = YAbs / float(n_max(r.bottom - r.top, 1));
	OK;
}
//---------------------------------------------------------------------

}

//#endif // __WIN32__
#endif