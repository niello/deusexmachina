#ifdef __WIN32__

#include "OSWindowWin32.h"

#include <System/OSWindowClassWin32.h>
#include <System/Events/OSInput.h>

#include <Uxtheme.h>
#include <WindowsX.h>

#ifndef HID_USAGE_PAGE_GENERIC
#define HID_USAGE_PAGE_GENERIC		((USHORT) 0x01)
#endif
#ifndef HID_USAGE_GENERIC_MOUSE
#define HID_USAGE_GENERIC_MOUSE		((USHORT) 0x02)
#endif
#ifndef HID_USAGE_GENERIC_KEYBOARD
#define HID_USAGE_GENERIC_KEYBOARD	((USHORT) 0x06)
#endif

#define ACCEL_TOGGLEFULLSCREEN	1001
#define STYLE_WINDOWED			(WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE)
#define STYLE_FULLSCREEN		(WS_POPUP | WS_SYSMENU | WS_VISIBLE)
#define STYLE_CHILD				(WS_CHILD | WS_TABSTOP | WS_VISIBLE)

namespace Sys
{

COSWindowWin32::~COSWindowWin32()
{
	if (Flags.Is(Wnd_Open)) Close();

	if (hAccel)
	{
		DestroyAcceleratorTable(hAccel);
		hAccel = NULL;
	}
}
//---------------------------------------------------------------------

bool COSWindowWin32::Open()
{
	n_assert(!hWnd && (!pParent || pParent->GetHWND()));

	if (Flags.Is(Wnd_Open)) OK;

	LONG WndStyle;
	RECT r;
	if (pParent)
	{
		WndStyle = STYLE_CHILD;
		::GetClientRect(pParent->GetHWND(), &r);
	}
	else
	{
		WndStyle = Flags.Is(Wnd_Fullscreen) ? STYLE_FULLSCREEN : STYLE_WINDOWED;
		r.left = Rect.Left();
		r.top = Rect.Top();
		r.right = Rect.Right();
		r.bottom = Rect.Bottom();
		::AdjustWindowRect(&r, WndStyle, FALSE);
	}

	hWnd = ::CreateWindowEx(Flags.Is(Wnd_Topmost) ? WS_EX_TOPMOST : 0,
							(const char*)(DWORD_PTR)WndClass->GetWin32WindowClass(), WindowTitle.CStr(), WndStyle,
							r.left, r.top, r.right - r.left, r.bottom - r.top,
							pParent ? pParent->GetHWND() : NULL, NULL, WndClass->GetWin32HInstance(), NULL);
	if (!hWnd) FAIL;

	::SetWindowLongPtr(hWnd, 0, (LONG)this);

	::ShowWindow(hWnd, SW_SHOWDEFAULT);

	::GetClientRect(hWnd, &r);
	Rect.W = r.right - r.left;
	Rect.H = r.bottom - r.top;

	POINT p;
	p.x = r.left;
	p.y = r.top;
	::ClientToScreen(hWnd, &p);
	Rect.X = p.x;
	Rect.Y = p.y;

	if (!hAccel)
	{
		ACCEL Acc[1];
		Acc[0].fVirt = FALT | FNOINVERT | FVIRTKEY;
		Acc[0].key = VK_RETURN;
		Acc[0].cmd = ACCEL_TOGGLEFULLSCREEN;
		hAccel = CreateAcceleratorTable(Acc, 1);
		n_assert(hAccel);
	}

	Flags.Set(Wnd_Open);
	Flags.SetTo(Wnd_Minimized, ::IsIconic(hWnd) == TRUE);

	FireEvent(CStrID("OnOpened"));

	OK;
}
//---------------------------------------------------------------------

void COSWindowWin32::Close()
{
	if (!Flags.Is(Wnd_Open)) return;
	::SendMessage(hWnd, WM_CLOSE, 0, 0);
}
//---------------------------------------------------------------------

void COSWindowWin32::Minimize()
{
	n_assert_dbg(hWnd && Flags.Is(Wnd_Open));
	if (!Flags.Is(Wnd_Minimized) && !pParent)
		::ShowWindow(hWnd, SW_MINIMIZE);
}
//---------------------------------------------------------------------

void COSWindowWin32::Restore()
{
	n_assert_dbg(hWnd && Flags.Is(Wnd_Open));
	::ShowWindow(hWnd, SW_RESTORE);
}
//---------------------------------------------------------------------

bool COSWindowWin32::SetRect(const Data::CRect& NewRect, bool FullscreenMode)
{
	if (!hWnd)
	{
		Rect = NewRect;
		OK;
	}

	//???set empty rect values to default? zero w & h at least.

	UINT SWPFlags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS;

	LONG PrevWndStyle = (LONG)::GetWindowLongPtr(hWnd, GWL_STYLE);
	LONG NewWndStyle = pParent ? STYLE_CHILD : (FullscreenMode ? STYLE_FULLSCREEN : STYLE_WINDOWED);
	
	if (NewWndStyle != PrevWndStyle)
	{
		if (::SetWindowLongPtr(hWnd, GWL_STYLE, NewWndStyle) == 0) FAIL;
		Flags.SetTo(Wnd_Fullscreen, FullscreenMode && !pParent);

		SWPFlags |= SWP_FRAMECHANGED;
		
		// Fullscreen mode breaks theme (at least aero glass) on Win7, so restore it when returned from the fullscreen mode
		if (PrevWndStyle == STYLE_FULLSCREEN) ::SetWindowTheme(hWnd, NULL, NULL);
	}

	if (Rect.X == NewRect.X && Rect.Y == NewRect.Y) SWPFlags |= SWP_NOMOVE;
	if (Rect.W == NewRect.W && Rect.H == NewRect.H) SWPFlags |= SWP_NOSIZE;

	RECT r = { NewRect.Left(), NewRect.Top(), NewRect.Right(), NewRect.Bottom() };
	::AdjustWindowRect(&r, NewWndStyle, FALSE);

	// Rect is updated in a window procedure
	if (::SetWindowPos(hWnd, NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWPFlags) == FALSE) FAIL;

	OK;
}
//---------------------------------------------------------------------

void COSWindowWin32::SetWindowClass(COSWindowClassWin32& WindowClass)
{
	n_assert2(!hWnd, "COSWindowWin32::SetWindowClass() > Can't change a class of a created window");
	WndClass = &WindowClass;
}
//---------------------------------------------------------------------

void COSWindowWin32::SetTitle(const char* pTitle)
{
	WindowTitle = pTitle;
	if (hWnd) ::SetWindowText(hWnd, pTitle);
}
//---------------------------------------------------------------------

// Changes class icon, but needs hWnd, so placed here
void COSWindowWin32::SetIcon(const char* pIconName)
{
	IconName = pIconName;
	if (hWnd && IconName.IsValid())
	{
		HICON hIcon = ::LoadIcon(WndClass->GetWin32HInstance(), IconName.CStr());
		if (hIcon) ::SetClassLongPtr(hWnd, GCLP_HICON, (LONG_PTR)hIcon);
	}
}
//---------------------------------------------------------------------

bool COSWindowWin32::SetTopmost(bool Topmost)
{
	if (pParent && Topmost) FAIL; //???or allow?
	if (Flags.Is(Wnd_Topmost) == Topmost) OK;

	if (hWnd)
	{
		HWND hWndInsertAfter;
		if (Topmost) hWndInsertAfter = HWND_TOPMOST;
		else if (pParent) hWndInsertAfter = pParent->GetHWND();
		else hWndInsertAfter = HWND_NOTOPMOST;
	
		if (::SetWindowPos(hWnd, hWndInsertAfter, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE) == FALSE) FAIL;
	}
	
	Flags.SetTo(Wnd_Topmost, Topmost);
	OK;
}
//---------------------------------------------------------------------

bool COSWindowWin32::SetInputFocus()
{
	return hWnd && (::SetFocus(hWnd) != NULL);
}
//---------------------------------------------------------------------

LONG COSWindowWin32::GetWin32Style() const
{
	n_assert(hWnd);
	return (LONG)::GetWindowLongPtr(hWnd, GWL_STYLE); //GWL_EXSTYLE
}
//---------------------------------------------------------------------

static inline void ProcessKey(U8& ScanCode, UINT& VirtualKey)
{
	// Corrections from:
	// https://blog.molecular-matters.com/2011/09/05/properly-handling-keyboard-input/
	//!!!see more there, implement!
	if (VirtualKey == VK_SHIFT)
		VirtualKey = ::MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK_EX);
	else if (VirtualKey == VK_NUMLOCK)
		ScanCode = (::MapVirtualKey(VirtualKey, MAPVK_VK_TO_VSC) | 0x100);
}
//---------------------------------------------------------------------

bool COSWindowWin32::HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& Result)
{
	switch (uMsg)
	{
		case WM_SYSCOMMAND:
			// Prevent moving/sizing and power loss in fullscreen mode
			if (Flags.Is(Wnd_Fullscreen))
			{
				switch (wParam)
				{
					case SC_MOVE:
					case SC_SIZE:
					case SC_MAXIMIZE:
					case SC_KEYMENU:
					case SC_MONITORPOWER:
						Result = 0; //1
						OK;
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ACCEL_TOGGLEFULLSCREEN:
					FireEvent(CStrID("OnToggleFullscreen"));
					break;
			}
			break;

		case WM_PAINT:
			FireEvent(CStrID("OnPaint"));
			break;

		case WM_ERASEBKGND:
			// Prevent Windows from erasing the background
			Result = 1;
			OK;

		case WM_MOVE:
		{
			IPTR X = (IPTR)LOWORD(lParam);
			IPTR Y = (IPTR)HIWORD(lParam);

			if (Rect.X != X || Rect.Y != Y)
			{
				Rect.X = X;
				Rect.Y = Y;
				FireEvent(CStrID("OnMoved"));
			}
			
			break;
		}

		//???does a child window receive this message from, say, .NET control? how to catch and handle resizing on parent resize?
		case WM_SIZE:
		{	
			if (wParam == SIZE_MAXHIDE || wParam == SIZE_MINIMIZED)
			{
				if (!Flags.Is(Wnd_Minimized))
				{
					Flags.Set(Wnd_Minimized);
					FireEvent(CStrID("OnMinimized"));
					::ReleaseCapture();
				}
			}
			else
			{
				if (Flags.Is(Wnd_Minimized))
				{
					Flags.Clear(Wnd_Minimized);
					FireEvent(CStrID("OnRestored"));
					::ReleaseCapture();
				}

				UPTR W = (UPTR)LOWORD(lParam);
				UPTR H = (UPTR)HIWORD(lParam);

				if (Rect.W != W || Rect.H != H)
				{
					Rect.W = W;
					Rect.H = H;
					FireEvent(CStrID("OnSizeChanged"));
				}
			}
			
			break;
		}

		case WM_SETCURSOR:
			if (FireEvent(CStrID("OnSetCursor")))
			{
				Result = TRUE;
				OK;
			}
			break;

		case WM_SETFOCUS:
		{
			FireEvent(CStrID("OnSetFocus"));
			::ReleaseCapture();

			//???here or globally? Globally no inputsink flag and hWnd = NULL
			//RIDEV_INPUTSINK - receive input even if not foreground, must specify hWnd
			//RIDEV_NOLEGACY - to prevent WM_KEYDOWN etc generation
			//???or switch in WM_MOUSEMOVE?
			RAWINPUTDEVICE RawInputDevices;
			RawInputDevices.usUsagePage = HID_USAGE_PAGE_GENERIC; 
			RawInputDevices.usUsage = HID_USAGE_GENERIC_MOUSE; 
			RawInputDevices.dwFlags = RIDEV_INPUTSINK;
			RawInputDevices.hwndTarget = hWnd;
			if (::RegisterRawInputDevices(&RawInputDevices, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
				Sys::Log("COSWindowWin32: High-definition (raw) mouse device registration failed!\n");

			//!!!DBG TMP!
			//Sys::DbgOut("WM_SETFOCUS\n");

			break;
		}

		case WM_KILLFOCUS:
			FireEvent(CStrID("OnKillFocus"));
			::ReleaseCapture();
			break;

		case WM_CLOSE:
			FireEvent(CStrID("OnClosing"));
			::DestroyWindow(hWnd);
			Result = 0;
			OK;

		case WM_DESTROY:
			Flags.Clear(Wnd_Open);
			FireEvent(CStrID("OnClosed"));
			hWnd = NULL;
			Result = 0;
			OK;

		case WM_KEYDOWN:
		{
			// If this keydown generated WM_CHAR, skip it and fire event from WM_CHAR
			MSG NextMessage;
			if (!::PeekMessage(&NextMessage, hWnd, 0, 0, PM_NOREMOVE) || NextMessage.message != WM_CHAR)
			{
				U8 ScanCode = ((U8*)&lParam)[2];
				UINT VirtualKey = wParam;
				ProcessKey(ScanCode, VirtualKey);

				Event::OSInput Ev;
				Ev.Type = Event::OSInput::KeyDown;
				Ev.KeyboardInfo.ScanCode = ScanCode;
				Ev.KeyboardInfo.VirtualKey = VirtualKey;
				Ev.KeyboardInfo.Char = 0;
				Ev.KeyboardInfo.IsRepeated = ((lParam & (1 << 30)) != 0);
				//if (lParam & (1 << 24)) Ev.KeyCode = (Ev.KeyCode | 0x80); // Extended key //???need to change scan code?
				FireEvent(Ev, Events::Event_TermOnHandled);
			}
			break;
		}

		case WM_KEYUP:
		{
			U8 ScanCode = ((U8*)&lParam)[2];
			UINT VirtualKey = wParam;
			ProcessKey(ScanCode, VirtualKey);

			Event::OSInput Ev;
			Ev.Type = Event::OSInput::KeyUp;
			Ev.KeyboardInfo.ScanCode = ScanCode;
			Ev.KeyboardInfo.VirtualKey = VirtualKey;
			Ev.KeyboardInfo.Char = 0;
			Ev.KeyboardInfo.IsRepeated = false;
			//if (lParam & (1 << 24)) Ev.KeyCode = (Ev.KeyCode | 0x80); // Extended key //???need to change scan code?
			FireEvent(Ev, Events::Event_TermOnHandled);
			break;
		}

		case WM_CHAR:
		case WM_UNICHAR:
		{
			n_assert_dbg(uMsg != WM_UNICHAR); //!!!implement!

			//???is valid? WM_CHAR must use UTF-16 itself. Always?
			WCHAR CharUTF16[2];
			::MultiByteToWideChar(CP_ACP, 0, (const char*)&wParam, 1, CharUTF16, 1);

			U8 ScanCode = ((U8*)&lParam)[2];
			UINT VirtualKey = ::MapVirtualKey(ScanCode, MAPVK_VSC_TO_VK_EX);
			//ProcessKey(ScanCode, VirtualKey); //???maps left shift twice?

			Event::OSInput Ev;
			Ev.Type = Event::OSInput::KeyDown;
			Ev.KeyboardInfo.ScanCode = ScanCode;
			Ev.KeyboardInfo.VirtualKey = VirtualKey;
			Ev.KeyboardInfo.Char = CharUTF16[0];
			Ev.KeyboardInfo.IsRepeated = ((lParam & (1 << 30)) != 0);
			//if (lParam & (1 << 24)) Ev.KeyCode = (Ev.KeyCode | 0x80); // Extended key //???need to change scan code?
			FireEvent(Ev, Events::Event_TermOnHandled);
			break;
		}

		case WM_INPUT:
		{
			RAWINPUT Data;
			PRAWINPUT pData = &Data;
			UINT DataSize = sizeof(Data);

			// IsForeground: GET_RAWINPUT_CODE_WPARAM(wParam) == RIM_INPUT

			if (::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, pData, &DataSize, sizeof(RAWINPUTHEADER)) == (UINT)-1) FAIL;

			//!!!???process mouse buttons here?!
			//???call defproc if processed?
			//!!!can process all devices here and send UID in an OSInput event!
			if (Data.header.dwType == RIM_TYPEMOUSE && (Data.data.mouse.lLastX || Data.data.mouse.lLastY)) 
			{
				Event::OSInput Ev;
				Ev.Type = Event::OSInput::MouseMoveRaw;
				Ev.MouseInfo.x = Data.data.mouse.lLastX;
				Ev.MouseInfo.y = Data.data.mouse.lLastY;
				FireEvent(Ev);
			}

			::DefRawInputProc(&pData, 1, sizeof(RAWINPUTHEADER));
			Result = 0;
			OK;
		}

		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			if (pParent) ::SetFocus(hWnd);

			Event::OSInput Ev;

			switch (uMsg)
			{
				case WM_LBUTTONDBLCLK:
				case WM_RBUTTONDBLCLK:
				case WM_MBUTTONDBLCLK:
				case WM_XBUTTONDBLCLK:
					Ev.Type = Event::OSInput::MouseDblClick;
					break;

				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
				case WM_MBUTTONDOWN:
				case WM_XBUTTONDOWN:
					Ev.Type = Event::OSInput::MouseDown;
					::SetCapture(hWnd);
					break;

				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
				case WM_XBUTTONUP:
					Ev.Type = Event::OSInput::MouseUp;
					::ReleaseCapture();
					break;
			}

			switch (uMsg)
			{
				case WM_LBUTTONDBLCLK:
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
					Ev.MouseInfo.Button = 0;
					break;

				case WM_RBUTTONDBLCLK:
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
					Ev.MouseInfo.Button = 1;
					break;

				case WM_MBUTTONDBLCLK:
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
					Ev.MouseInfo.Button = 2;
					break;

				case WM_XBUTTONDBLCLK:
				case WM_XBUTTONDOWN:
				case WM_XBUTTONUP:
				{
					switch (GET_XBUTTON_WPARAM(wParam))
					{
						case XBUTTON1:	Ev.MouseInfo.Button = 3; break;
						case XBUTTON2:	Ev.MouseInfo.Button = 4; break;
						default:		Ev.MouseInfo.Button = 5; break;
					}
					break;
				}
			}

			Ev.MouseInfo.x = GET_X_LPARAM(lParam);
			Ev.MouseInfo.y = GET_Y_LPARAM(lParam);
			FireEvent(Ev);
			break;
		}

		case WM_MOUSEMOVE:
		{
			Event::OSInput Ev;
			Ev.Type = Event::OSInput::MouseMove;
			Ev.MouseInfo.x = GET_X_LPARAM(lParam);
			Ev.MouseInfo.y = GET_Y_LPARAM(lParam);
			FireEvent(Ev);
			break;
		}

		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
		{
			Event::OSInput Ev;
			Ev.Type = (uMsg == WM_MOUSEWHEEL) ? Event::OSInput::MouseWheelVertical : Event::OSInput::MouseWheelHorizontal;
			Ev.WheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			FireEvent(Ev);
			break;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

}

#endif //__WIN32__
