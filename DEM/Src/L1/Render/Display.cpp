//#ifdef __WIN32__
#include "Display.h"

#include <Render/RenderServer.h>
#include <Events/EventManager.h>
#include <Gfx/Events/DisplayInput.h>
//#include "kernel/nkernelserver.h"

//???in what header?
#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC	((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE	((USHORT) 0x02)
#endif

CDisplay::CDisplay():
	WindowTitle("DeusExMachina - Untitled"),
	IsWndOpen(false),
	Fullscreen(false),
	VSync(false),
	AlwaysOnTop(false),
	AutoAdjustSize(true),
	DisplayModeSwitchEnabled(true),
	TripleBuffering(false),
	Adapter(Adapter_Primary),
	AntiAliasQuality(MSAA_None),
	hInst(NULL),
	hWnd(NULL),
	hWndParent(NULL),
	hAccel(NULL),
	aWndClass(0),
	StyleWindowed(WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE),
	StyleFullscreen(WS_POPUP | WS_SYSMENU | WS_VISIBLE),
	StyleChild(WS_CHILD | WS_VISIBLE) //WS_CHILD | WS_TABSTOP - N2, now N3 variant is selected
{
	hInst = GetModuleHandle(NULL);
}
//---------------------------------------------------------------------

CDisplay::~CDisplay()
{
	if (IsWndOpen) CloseWindow();

	if (hAccel)
	{
		DestroyAcceleratorTable(hAccel);
		hAccel = NULL;
	}

	if (aWndClass)
	{
		if (!UnregisterClass((LPCSTR)aWndClass, hInst))
			n_error("CDisplay::CloseWindow(): UnregisterClass() failed!\n");
		aWndClass = 0;
	}
}
//---------------------------------------------------------------------

bool CDisplay::OpenWindow()
{
	n_assert(!IsWndOpen && hInst && !hWnd);

	// Send DisplayOpen event

	// Calculate adjusted window rect

	//!!!in N3 this is done through SetParentWindow!
	// Parent HWND handling
	if (CoreSrv->GetGlobal<int>("parent_hwnd", (int&)hWndParent))
	{
		RECT r;
		GetClientRect(hWndParent, &r);
		DisplayMode.Width = (ushort)(r.right - r.left);
		DisplayMode.Height = (ushort)(r.bottom - r.top);
	}
	else hWndParent = NULL;

	if (!hAccel)
	{
		ACCEL Acc[1];
		Acc[0].fVirt = FALT | FNOINVERT | FVIRTKEY;
		Acc[0].key = VK_RETURN;
		Acc[0].cmd = ACCEL_TOGGLEFULLSCREEN;
		hAccel = CreateAcceleratorTable(Acc, 1);
		n_assert(hAccel);
	}

	if (!aWndClass)
	{
		HICON hIcon = NULL;
		if (IconName.IsValid()) hIcon = LoadIcon(hInst, IconName.Get());
		if (!hIcon) hIcon = LoadIcon(NULL, IDI_APPLICATION);

		WNDCLASSEX WndClass;
		memset(&WndClass, 0, sizeof(WndClass));
		WndClass.cbSize        = sizeof(WndClass);
		WndClass.style         = CS_DBLCLKS;
		WndClass.lpfnWndProc   = WinProc;
		WndClass.cbClsExtra    = 0;
		WndClass.cbWndExtra    = sizeof(void*);   // used to hold 'this' pointer
		WndClass.hInstance     = hInst;
		WndClass.hIcon         = hIcon;
		WndClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		WndClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		WndClass.lpszMenuName  = NULL;
		WndClass.lpszClassName = DEM_WINDOW_CLASS;
		WndClass.hIconSm       = NULL;
		aWndClass = RegisterClassEx(&WndClass);
		n_assert(aWndClass);
	}

	DWORD WndStyle;
	if (hWndParent) WndStyle = StyleChild;
	else if (Fullscreen) WndStyle = StyleFullscreen;
	else WndStyle = StyleWindowed;

	int X, Y, W, H;
	CalcWindowRect(X, Y, W, H);

	hWnd = CreateWindow((LPCSTR)aWndClass, WindowTitle.Get(), WndStyle,
						X, Y, W, H,
						hWndParent, NULL, hInst, NULL);
	n_assert(hWnd);

	if (AlwaysOnTop) SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

	SetWindowLong(hWnd, 0, (LONG)this);

	CoreSrv->SetGlobal("hwnd", (int)hWnd);

	RAWINPUTDEVICE RawInputDevices[1];
	RawInputDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
	RawInputDevices[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
	RawInputDevices[0].dwFlags = RIDEV_INPUTSINK;
	RawInputDevices[0].hwndTarget = hWnd;

	if (RegisterRawInputDevices(RawInputDevices, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
		n_printf("CDisplay: High-definition (raw) mouse device registration failed!\n");

	IsWndOpen = true;
	IsWndMinimized = false;
	OK;
}
//---------------------------------------------------------------------

void CDisplay::CloseWindow()
{
	n_assert(IsWndOpen && hInst);

	// Close if not already closed externally by (e.g. by Alt-F4)
	if (hWnd)
	{
		DestroyWindow(hWnd);
		hWnd = NULL;
	}

	// send DisplayClose event

	IsWndOpen = false;
}
//---------------------------------------------------------------------

// Polls for and processes window messages. Call this message once per
// frame in your render loop. If the user clicks the window close
// button, or hits Alt-F4, a CloseRequested input event will be sent.
void CDisplay::ProcessWindowMessages()
{
	n_assert(IsWndOpen);

	// It may happen that the WinProc has already closed our window!
	if (!hWnd) return;

	// NB: we pass NULL instead of window handle to receive language switching messages
	MSG Msg;
	while (PeekMessage(&Msg, NULL /*hWnd*/, 0, 0, PM_REMOVE))
	{
		if (hAccel && TranslateAccelerator(hWnd, hAccel, &Msg) != FALSE) continue;
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
}
//---------------------------------------------------------------------

void CDisplay::SetWindowTitle(const char* pTitle)
{
	WindowTitle = pTitle;
	if (hWnd) SetWindowText(hWnd, pTitle);
}
//---------------------------------------------------------------------

void CDisplay::SetWindowIcon(const char* pIconName)
{
	IconName = pIconName;
	if (hWnd && IconName.IsValid())
	{
		HICON hIcon = LoadIcon(hInst, IconName.Get());
		if (hIcon) SetClassLong(hWnd, GCL_HICON, (LONG)hIcon);
	}
}
//---------------------------------------------------------------------

void CDisplay::CalcWindowRect(int& X, int& Y, int& W, int& H)
{
	if (hWndParent)
	{
		X = 0;
		Y = 0;

		RECT r;
		GetClientRect(hWndParent, &r);
		AdjustWindowRect(&r, StyleChild, FALSE);
		W = (ushort)(r.right - r.left); //???clamp w & h to parent rect?
		H = (ushort)(r.bottom - r.top);

		// Child window adjusts display mode values
		DisplayMode.PosX = X;
		DisplayMode.PosY = Y;
		DisplayMode.Width = W;
		DisplayMode.Height = H;
	}
	else
	{
		CMonitorInfo MonitorInfo;
		GetAdapterMonitorInfo(Adapter, MonitorInfo);

		if (Fullscreen)
		{
			if (DisplayModeSwitchEnabled)
			{
				X = MonitorInfo.Left;
				Y = MonitorInfo.Top;
			}
			else
			{
				X = MonitorInfo.Left + ((MonitorInfo.Width - DisplayMode.Width) / 2);
				Y = MonitorInfo.Top + ((MonitorInfo.Height - DisplayMode.Height) / 2);
			}
			W = DisplayMode.Width;
			H = DisplayMode.Height;
		}
		else
		{
			X = MonitorInfo.Left + DisplayMode.PosX;
			Y = MonitorInfo.Top + DisplayMode.PosY;
			RECT r = { X, Y, X + DisplayMode.Width, Y + DisplayMode.Height };
			AdjustWindowRect(&r, StyleWindowed, FALSE);
			W = r.right - r.left;
			H = r.bottom - r.top;
		}
	}
}
//---------------------------------------------------------------------

void CDisplay::RestoreWindow()
{
	n_assert(hWnd && IsWndOpen);

	ShowWindow(hWnd, SW_RESTORE);

	int X, Y, W, H;
	CalcWindowRect(X, Y, W, H);

	HWND hWndInsertAfter;
	if (AlwaysOnTop) hWndInsertAfter = HWND_TOPMOST;
	else if (hWndParent) hWndInsertAfter = hWndParent;
	else hWndInsertAfter = HWND_NOTOPMOST;

	SetWindowPos(hWnd, hWndInsertAfter, X, Y, W, H, SWP_SHOWWINDOW);

	//!!!IF WM_SIZE IS CALLED, REMOVE STRING BELOW!  (see WM_SIZE)
	// Manually change window size in child mode
	if (hWndParent) MoveWindow(hWnd, X, Y, W, H, TRUE);
	IsWndMinimized = false;
}
//---------------------------------------------------------------------

void CDisplay::MinimizeWindow()
{
	n_assert(hWnd && IsWndOpen);
	if (!IsWndMinimized)
	{
		if (!hWndParent) ShowWindow(hWnd, SW_MINIMIZE);
		IsWndMinimized = true;
	}
}
//---------------------------------------------------------------------

bool CDisplay::AdapterExists(EAdapter Adapter)
{
	return ((UINT)Adapter) < RenderSrv->GetD3D()->GetAdapterCount();
}
//---------------------------------------------------------------------

void CDisplay::GetAvailableDisplayModes(EAdapter Adapter, EPixelFormat Format, nArray<CDisplayMode>& OutModes)
{
	n_assert(AdapterExists(Adapter));
	D3DDISPLAYMODE D3DDisplayMode = { 0 }; 
	UINT ModeCount = RenderSrv->GetD3D()->GetAdapterModeCount(Adapter, Format);
	for (UINT i = 0; i < ModeCount; i++)
	{
		//HRESULT hr =
		RenderSrv->GetD3D()->EnumAdapterModes(Adapter, Format, i, &D3DDisplayMode);
		CDisplayMode Mode(D3DDisplayMode.Width, D3DDisplayMode.Height, D3DDisplayMode.Format);
		if (OutModes.FindIndex(Mode) == INVALID_INDEX)
			OutModes.Append(Mode);
	}
}
//---------------------------------------------------------------------

bool CDisplay::SupportsDisplayMode(EAdapter Adapter, const CDisplayMode& Mode)
{
	nArray<CDisplayMode> Modes;
	GetAvailableDisplayModes(Adapter, Mode.PixelFormat, Modes);
	return Modes.FindIndex(Mode) != INVALID_INDEX;
}
//---------------------------------------------------------------------

void CDisplay::GetCurrentAdapterDisplayMode(EAdapter Adapter, CDisplayMode& OutMode)
{
	n_assert(AdapterExists(Adapter));
	D3DDISPLAYMODE D3DDisplayMode = { 0 }; 
	HRESULT hr = RenderSrv->GetD3D()->GetAdapterDisplayMode((UINT)Adapter, &D3DDisplayMode);
	n_assert(SUCCEEDED(hr));
	OutMode.PosX = 0;
	OutMode.PosY = 0;
	OutMode.Width = D3DDisplayMode.Width;
	OutMode.Height = D3DDisplayMode.Height;
	OutMode.PixelFormat = D3DDisplayMode.Format;
}
//---------------------------------------------------------------------

/*
AdapterInfo D3D9DisplayDevice::GetAdapterInfo(Adapter::Code adapter)
{
    n_assert(this->AdapterExists(adapter));
    IDirect3D9* d3d9 = D3D9RenderDevice::GetDirect3D();
    D3DADAPTER_IDENTIFIER9 d3dInfo = { 0 };
    HRESULT hr = d3d9->GetAdapterIdentifier((UINT) adapter, 0, &d3dInfo);
    n_assert(SUCCEEDED(hr));

    AdapterInfo info;
    info.SetDriverName(d3dInfo.Driver);
    info.SetDescription(d3dInfo.Description);
    info.SetDeviceName(d3dInfo.DeviceName);
    info.SetDriverVersionLowPart(d3dInfo.DriverVersion.LowPart);
    info.SetDriverVersionHighPart(d3dInfo.DriverVersion.HighPart);
    info.SetVendorId(d3dInfo.VendorId);
    info.SetDeviceId(d3dInfo.DeviceId);
    info.SetSubSystemId(d3dInfo.SubSysId);
    info.SetRevision(d3dInfo.Revision);
    info.SetGuid(Guid((const unsigned char*) &(d3dInfo.DeviceIdentifier), sizeof(d3dInfo.DeviceIdentifier)));
    return info;
}
*/

void CDisplay::GetAdapterMonitorInfo(EAdapter Adapter, CMonitorInfo& OutInfo)
{
	n_assert(AdapterExists(Adapter));
	HMONITOR hMonitor = RenderSrv->GetD3D()->GetAdapterMonitor(Adapter);
	MONITORINFO Win32MonitorInfo = { sizeof(Win32MonitorInfo), 0 };
	GetMonitorInfo(hMonitor, &Win32MonitorInfo);
	OutInfo.Left = (ushort)Win32MonitorInfo.rcMonitor.left;
	OutInfo.Top = (ushort)Win32MonitorInfo.rcMonitor.top;
	OutInfo.Width = (ushort)(Win32MonitorInfo.rcMonitor.right - Win32MonitorInfo.rcMonitor.left);
	OutInfo.Height = (ushort)(Win32MonitorInfo.rcMonitor.bottom - Win32MonitorInfo.rcMonitor.top);
	OutInfo.IsPrimary = Win32MonitorInfo.dwFlags & MONITORINFOF_PRIMARY;
}
//---------------------------------------------------------------------

LONG WINAPI CDisplay::WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CDisplay* pDisp = (CDisplay*)GetWindowLong(hWnd, 0);
	LONG Result = 0;
	if (pDisp->HandleWindowMessage(hWnd, uMsg, wParam, lParam, Result)) return Result;
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
//---------------------------------------------------------------------

//???need _hWnd param?
bool CDisplay::HandleWindowMessage(HWND _hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LONG Result)
{
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
			// Prevent moving/sizing and power loss in fullscreen mode
			if (Fullscreen)
			{
				switch (wParam)
				{
					case SC_MOVE:
					case SC_SIZE:
					case SC_MAXIMIZE:
					case SC_KEYMENU:
					case SC_MONITORPOWER:
						Result = 1;
						OK;
				}
			}
			break;

		case WM_ERASEBKGND:
			// Prevent Windows from erasing the background
			Result = 1;
			OK;

		case WM_SIZE:
			if (wParam == SIZE_MAXHIDE || wParam == SIZE_MINIMIZED)
			{
				IsWndMinimized = true;
				EventMgr->FireEvent(CStrID("OnDisplayMinimized"));
				ReleaseCapture();
			}
			else
			{
				IsWndMinimized = false;
				EventMgr->FireEvent(CStrID("OnDisplayRestored"));
				// As a child window, do not release capture, because it would block the resizing
				if (!hWndParent) ReleaseCapture();
				if (hWnd && AutoAdjustSize) AdjustSize();
			}
			// Manually change window size in child mode
			//!!!x & y were 0
			if (hWndParent) MoveWindow(hWnd, DisplayMode.PosX, DisplayMode.PosY, LOWORD(lParam), HIWORD(lParam), TRUE);
			break;

		case WM_SETCURSOR:
			if (EventMgr->FireEvent(CStrID("OnDisplaySetCursor")))
			{
				Result = TRUE;
				OK;
			}
			break;

		case WM_PAINT:
			EventMgr->FireEvent(CStrID("OnDisplayPaint"));
			break;

		case WM_SETFOCUS:
			EventMgr->FireEvent(CStrID("OnDisplaySetFocus"));
			ReleaseCapture();
			break;

		case WM_KILLFOCUS:
			EventMgr->FireEvent(CStrID("OnDisplayKillFocus"));
			ReleaseCapture();
			break;

		case WM_CLOSE:
			EventMgr->FireEvent(CStrID("OnDisplayClose"));
			hWnd = NULL;
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ACCEL_TOGGLEFULLSCREEN:
					EventMgr->FireEvent(CStrID("OnDisplayToggleFullscreen"));
					break;
			}
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Event::DisplayInput Ev;
			Ev.Type = (uMsg == WM_KEYDOWN) ? Event::DisplayInput::KeyDown : Event::DisplayInput::KeyUp;
			Ev.KeyCode = (Input::EKey)((uchar*)&lParam)[2];
			if (lParam & (1 << 24)) Ev.KeyCode = (Input::EKey)(Ev.KeyCode | 0x80);
			EventMgr->FireEvent(Ev);
			break;
		}

		case WM_CHAR:
		{
			WCHAR CharUTF16[2];
			MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&wParam, 1, CharUTF16, 1);

			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::CharInput;
			Ev.Char = CharUTF16[0];
			EventMgr->FireEvent(Ev);
			break;
		}

		case WM_INPUT:
		{
			RAWINPUT Data;
			PRAWINPUT pData = &Data;
			UINT DataSize = sizeof(Data);

			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pData, &DataSize, sizeof(RAWINPUTHEADER));

			if (Data.header.dwType == RIM_TYPEMOUSE && (Data.data.mouse.lLastX || Data.data.mouse.lLastY)) 
			{
				Event::DisplayInput Ev;
				Ev.Type = Event::DisplayInput::MouseMoveRaw;
				Ev.MouseInfo.x = Data.data.mouse.lLastX;
				Ev.MouseInfo.y = Data.data.mouse.lLastY;
				EventMgr->FireEvent(Ev);
			}

			DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));
			Result = 0;
			OK;
		}

		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			if (hWndParent) SetFocus(hWnd);

			Event::DisplayInput Ev;

			switch (uMsg)
			{
				case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDBLCLK:
					Ev.Type = Event::DisplayInput::MouseDblClick;
					break;

				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
				case WM_MBUTTONDOWN:
					Ev.Type = Event::DisplayInput::MouseDown;
					SetCapture(hWnd);
					break;

				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
					Ev.Type = Event::DisplayInput::MouseUp;
					ReleaseCapture();
					break;
			}

			switch (uMsg)
			{
				case WM_LBUTTONDBLCLK:
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
					Ev.MouseInfo.Button = Input::MBLeft;
					break;

				case WM_RBUTTONDBLCLK:
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
					Ev.MouseInfo.Button = Input::MBRight;
					break;

				case WM_MBUTTONDBLCLK:
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
					Ev.MouseInfo.Button = Input::MBMiddle;
					break;
			}

			Ev.MouseInfo.x = LOWORD(lParam);
			Ev.MouseInfo.y = HIWORD(lParam);
			EventMgr->FireEvent(Ev);
			break;
		}

		case WM_MOUSEMOVE:
		{
			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::MouseMove;
			Ev.MouseInfo.x = LOWORD(lParam);
			Ev.MouseInfo.y = HIWORD(lParam);
			EventMgr->FireEvent(Ev);
			break;
		}

		case WM_MOUSEWHEEL:
		{
			Event::DisplayInput Ev;
			Ev.Type = Event::DisplayInput::MouseWheel;
			Ev.WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			EventMgr->FireEvent(Ev);
			break;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

//???change here & listen event in D3D9 handler, compare D3D9 settings with curr display mode & reset device if needed>?
void CDisplay::AdjustSize()
{
	n_assert(hWnd);

	float OldW = DisplayMode.Width;
	float OldH = DisplayMode.Height;

	RECT r;
	GetClientRect(hWnd, &r);
	DisplayMode.Width = (ushort)r.right;
	DisplayMode.Height = (ushort)r.bottom;

	if (DisplayMode.Width != OldW || DisplayMode.Height != OldH)
		RenderSrv->ResetDevice();

	//???EventMgr->FireEvent(CStrID("OnDisplaySizeChanged")); and handle in device?
}
//---------------------------------------------------------------------

//#endif //__WIN32__
