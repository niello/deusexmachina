#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_H__
#define __DEM_L1_RENDER_DISPLAY_H__
//#ifdef __WIN32__

#include <Render/DisplayMode.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// A window handler for Win32 platforms.

#define DEM_WINDOW_CLASS "DeusExMachina::MainWindow"

using namespace Render;

class CDisplay
{
public:

	//???UINT and defines for primary & secondary instead of enum?
	enum EAdapter
	{
		Adapter_None = -1,
		Adapter_Primary = 0,
		Adapter_Secondary = 1
	};

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
	LONG			StyleWindowed;		///< WS_* flags for windowed mode
	LONG			StyleFullscreen;	///< WS_* flags for full-screen mode
	LONG			StyleChild;			///< WS_* flags for child mode

	bool				HandleWindowMessage(HWND _hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG Result);
	void				CalcWindowRect(int& X, int& Y, int& W, int& H);

	static LONG WINAPI	WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	EAdapter		Adapter;
	bool			Fullscreen;
	bool			VSync;
	bool			AlwaysOnTop;
	bool			AutoAdjustSize;				// Autoadjust viewport (display mode W & H) when window size changes
	bool			DisplayModeSwitchEnabled;	//???
	bool			TripleBuffering;			// Use double or triple buffering when fullscreen
	EMSAAQuality	AntiAliasQuality;

	CDisplay();
	~CDisplay();

	bool				OpenWindow();
	void				CloseWindow();
	void				ProcessWindowMessages();
	void				ResetWindow();
	void				RestoreWindow();
	void				MinimizeWindow();
	void				AdjustSize();

	bool				AdapterExists(EAdapter Adapter);
	void				GetAvailableDisplayModes(EAdapter Adapter, EPixelFormat Format, nArray<CDisplayMode>& OutModes);
	bool				SupportsDisplayMode(EAdapter Adapter, const CDisplayMode& Mode);
	bool				GetCurrentAdapterDisplayMode(EAdapter Adapter, CDisplayMode& OutMode);
	//CAdapterInfo		GetAdapterInfo(EAdapter Adapter);
	void				GetAdapterMonitorInfo(EAdapter Adapter, CMonitorInfo& OutInfo);

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

inline void CDisplay::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
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

//#endif // __WIN32__
#endif