#ifndef N_WIN32WINDOWHANDLER_H
#define N_WIN32WINDOWHANDLER_H
#ifdef __WIN32__

#include <gfx2/DisplayMode.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// A window handler for Win32 platforms. Contains the window handling code both for the
// Direct3D and OpenGL graphics servers (if running under Windows).
// (C) 2004 RadonLabs GmbH

#define NEBULA2_WINDOW_CLASS "Nebula2::MainWindow"

class CWin32Display //???event dispatcher? or can send through EventMgr!
{
protected:

	enum
	{
		ACCEL_TOGGLEFULLSCREEN = 1001,
	};

	nString			WindowTitle;
	nString			IconName;
	CDisplayMode	DisplayMode;
	bool			IsWndOpen;
	bool			IsWndMinimized;

	HINSTANCE		hInst;
	HWND			hWnd;
	HWND			hWndParent;
	HACCEL			hAccel;
	ATOM			aWndClass;
	DWORD			StyleWindowed;		///< WS_* flags for windowed mode
	DWORD			StyleFullscreen;	///< WS_* flags for full-screen mode
	DWORD			StyleChild;			///< WS_* flags for child mode

	bool				HandleWindowMessage(HWND _hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG Result);
	void				CalcWindowRect(int& W, int& H);

	static LONG WINAPI	WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	bool			Fullscreen;
	bool			AlwaysOnTop;
	bool			AutoAdjustSize;

	CWin32Display();
	~CWin32Display();

	bool				OpenWindow();
	void				CloseWindow();
	void				ProcessWindowMessages();
	void				RestoreWindow();
	void				MinimizeWindow();
	void				AdjustSize();

	void				SetWindowTitle(const char* pTitle);
	const nString&		GetWindowTitle() const { return WindowTitle; }
	void				SetWindowIcon(const char* pIconName);
	const nString&		GetWindowIcon() const { return IconName; }
	void				SetDisplayMode(const CDisplayMode& DispMode) { DisplayMode = DispMode; }
	const CDisplayMode&	GetDisplayMode() const { return DisplayMode; }

	void				GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	bool				IsWindowOpen() const { return IsWndOpen; }
	bool				IsWindowMinimized() const { return IsWndMinimized; }
	HWND				GetAppHwnd() const { return hWnd; }
	void				SetParentHwnd(HWND Parent) { hWndParent = Parent; }
	HWND				GetParentHwnd() const { return hWndParent; }
	ATOM				GetWndClass() const { return aWndClass; }
};

inline void CWin32Display::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	RECT r;
	if (hWnd && GetClientRect(hWnd, &r))
	{
		XRel = XAbs / float(n_max(r.right - r.left, 1));
		YRel = YAbs / float(n_max(r.bottom - r.top, 1));
	}
	else
	{
		XRel = XAbs / float(DisplayMode.Width);
		YRel = YAbs / float(DisplayMode.Height);
	}
}
//---------------------------------------------------------------------

#endif // __WIN32__
#endif
