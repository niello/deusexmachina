#if DEM_PLATFORM_WIN32
#include "OSWindowWin32.h"
#include <System/SystemEvents.h>

#include <Uxtheme.h>
#include <WindowsX.h>

#define STYLE_WINDOWED			(WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE)
#define STYLE_FULLSCREEN		(WS_POPUP | WS_SYSMENU | WS_VISIBLE)
#define STYLE_CHILD				(WS_CHILD | WS_TABSTOP | WS_VISIBLE)

//!!!DUPLICATE!
constexpr int ACCEL_TOGGLEFULLSCREEN = 1001;

namespace DEM { namespace Sys
{
__ImplementClassNoFactory(DEM::Sys::COSWindowWin32, Core::CObject);

COSWindowWin32::COSWindowWin32(HINSTANCE hInstance, ATOM aWndClass, COSWindowWin32* pParentWnd)
{
	LONG WndStyle;
	RECT r;
	if (pParentWnd)
	{
		n_assert(pParentWnd->GetHWND());
		WndStyle = STYLE_CHILD;
		::GetClientRect(pParentWnd->GetHWND(), &r);
		Flags.Clear(Wnd_Fullscreen);
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
		(const char*)(DWORD_PTR)aWndClass, "", WndStyle,
		r.left, r.top, r.right - r.left, r.bottom - r.top,
		pParentWnd ? pParentWnd->GetHWND() : NULL, NULL, hInstance, NULL);

	if (!hWnd) return;

	pParent = pParentWnd;

	::SetWindowLongPtr(hWnd, 0, (LONG_PTR)this);

	::GetClientRect(hWnd, &r);
	Rect.W = r.right - r.left;
	Rect.H = r.bottom - r.top;

	POINT p;
	p.x = r.left;
	p.y = r.top;
	::ClientToScreen(hWnd, &p);
	Rect.X = p.x;
	Rect.Y = p.y;

	// Set to initial state
	::ShowWindow(hWnd, SW_SHOWDEFAULT);
	Flags.SetTo(Wnd_Minimized, ::IsIconic(hWnd) == TRUE);

	FireEvent(CStrID("OnOpened"));
}
//---------------------------------------------------------------------

COSWindowWin32::~COSWindowWin32()
{
	if (hWnd)
	{
		::SetWindowLongPtr(hWnd, 0, 0);
		::SendMessage(hWnd, WM_CLOSE, 0, 0);
	}
}
//---------------------------------------------------------------------

bool COSWindowWin32::Show()
{
	n_assert_dbg(hWnd);
	if (!hWnd || ::ShowWindow(hWnd, SW_SHOW) == FALSE) FAIL;

	//???need to redraw frame right now?
	//::UpdateWindow(hWnd);
	// or
	//::RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT | RDW_UPDATENOW);

	// event, state?

	OK;
}
//---------------------------------------------------------------------

bool COSWindowWin32::Hide()
{
	n_assert_dbg(hWnd);
	if (!hWnd || ::ShowWindow(hWnd, SW_HIDE) == FALSE) FAIL;
	// event, state?
	OK;
}
//---------------------------------------------------------------------

void COSWindowWin32::Close()
{
	if (hWnd) ::SendMessage(hWnd, WM_CLOSE, 0, 0);
}
//---------------------------------------------------------------------

void COSWindowWin32::Minimize()
{
	n_assert_dbg(hWnd);
	if (hWnd && !Flags.Is(Wnd_Minimized) && !pParent)
		::ShowWindow(hWnd, SW_MINIMIZE);
}
//---------------------------------------------------------------------

void COSWindowWin32::Restore()
{
	n_assert_dbg(hWnd);
	if (hWnd) ::ShowWindow(hWnd, SW_RESTORE);
}
//---------------------------------------------------------------------

bool COSWindowWin32::SetRect(const Data::CRect& NewRect, bool FullscreenMode)
{
	n_assert_dbg(hWnd);
	if (!hWnd) FAIL;

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

void COSWindowWin32::SetTitle(const char* pTitle)
{
	n_assert_dbg(hWnd);
	if (hWnd) ::SetWindowText(hWnd, pTitle);
}
//---------------------------------------------------------------------

CString COSWindowWin32::GetTitle() const
{
	CString Out;
	if (!hWnd) return Out;

	const int Len = ::GetWindowTextLength(hWnd) + 1;
	Out.Reserve(Len);
	char* pBuf = n_new_array(char, Len);
	::GetWindowText(hWnd, pBuf, Len);
	Out = pBuf;
	n_delete_array(pBuf);
	return Out;
}
//---------------------------------------------------------------------

bool COSWindowWin32::GetCursorPosition(IPTR& OutX, IPTR& OutY) const
{
	if (!hWnd) FAIL;
	POINT Pos;
	if (!::GetCursorPos(&Pos)) FAIL;
	if (!::ScreenToClient(hWnd, &Pos)) FAIL;
	OutX = Pos.x;
	OutY = Pos.y;
	OK;
}
//---------------------------------------------------------------------

void COSWindowWin32::SetIcon(const char* pIconName)
{
	IconName = pIconName;
	if (hWnd && IconName.IsValid())
	{
		//???delete prev icon?
		HINSTANCE hInst = (HINSTANCE)::GetWindowLongPtr(hWnd, GWL_HINSTANCE);
		HICON hIcon = ::LoadIcon(hInst, IconName.CStr());
		if (hIcon) ::SendMessage(hWnd, WM_SETICON, ICON_BIG, (LONG_PTR)hIcon);
	}
}
//---------------------------------------------------------------------

bool COSWindowWin32::SetCursor(const char* pCursorName)
{
	// For now all cursors are loaded as IDC_ARROW, may rewrite later
	hClientCursor = pCursorName ? ::LoadCursor(NULL, IDC_ARROW) : NULL;
	OK;
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

bool COSWindowWin32::HasInputFocus() const
{
	if (!hWnd) FAIL;

	HWND hWndCheck = ::GetFocus();
	if (hWndCheck) return hWndCheck == hWnd;

	hWndCheck = ::GetActiveWindow();
	if (hWndCheck) return hWndCheck == hWnd;

	return ::GetForegroundWindow() == hWnd;
}
//---------------------------------------------------------------------

LONG COSWindowWin32::GetWin32Style() const
{
	n_assert(hWnd);
	return (LONG)::GetWindowLongPtr(hWnd, GWL_STYLE); //GWL_EXSTYLE
}
//---------------------------------------------------------------------

bool COSWindowWin32::HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& OutResult)
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
						OutResult = 0; //1
						OK;
				}
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ACCEL_TOGGLEFULLSCREEN:
					if (!pParent) FireEvent(CStrID("OnToggleFullscreen"));
					break;
			}
			break;

		case WM_PAINT:
			FireEvent(CStrID("OnPaint"));
			break;

		case WM_ERASEBKGND:
			// Prevent Windows from erasing the background
			OutResult = 1;
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

				const UPTR W = static_cast<UPTR>(LOWORD(lParam));
				const UPTR H = static_cast<UPTR>(HIWORD(lParam));

				if (Rect.W != W || Rect.H != H)
				{
					Event::OSWindowResized Ev(Rect.W, Rect.H, W, H, ManualResizingInProgress);
					
					Rect.W = W;
					Rect.H = H;
					
					FireEvent(Ev);
				}
			}

			break;
		}

		case WM_ENTERSIZEMOVE:
		{
			PrevWidth = Rect.W;
			PrevHeight = Rect.H;
			ManualResizingInProgress = true;
			break;
		}

		case WM_EXITSIZEMOVE:
		{
			ManualResizingInProgress = false;
			if (PrevWidth != Rect.W || PrevHeight != Rect.H)
				FireEvent(Event::OSWindowResized(PrevWidth, PrevHeight, Rect.W, Rect.H, false));
			break;
		}

		case WM_SETCURSOR:
		{
			if (LOWORD(lParam) == HTCLIENT)
			{
				::SetCursor(hClientCursor);
				FireEvent(CStrID("OnSetCursor"));
				OutResult = TRUE;
				OK;
			}
			break;
		}

		case WM_SETFOCUS:
			FireEvent(CStrID("OnSetFocus"));
			::ReleaseCapture();
			break;

		case WM_KILLFOCUS:
			FireEvent(CStrID("OnKillFocus"));
			::ReleaseCapture();
			break;

		case WM_CLOSE:
			FireEvent(CStrID("OnClosing"));
			::DestroyWindow(hWnd);
			OutResult = 0;
			OK;

		case WM_DESTROY:
			FireEvent(CStrID("OnClosed"));
			hWnd = NULL;
			OutResult = 0;
			OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

}}

#endif
