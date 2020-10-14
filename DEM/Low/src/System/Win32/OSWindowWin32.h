#pragma once
#if DEM_PLATFORM_WIN32
#include <System/OSWindow.h>
#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Win32 operating system window implementation

namespace DEM::Sys
{
typedef Ptr<class COSWindowWin32> POSWindowWin32;

class COSWindowWin32 : public COSWindow
{
	RTTI_CLASS_DECL(DEM::Sys::COSWindowWin32, Core::CObject);

protected:

	COSWindowWin32*	pParent = nullptr;
	HWND			hWnd = 0;
	HCURSOR			hClientCursor = 0;

	// For resizing detection in WM_EXITSIZEMOVE
	U16				PrevWidth = 0;
	U16				PrevHeight = 0;
	bool			ManualResizingInProgress = false;

public:

	COSWindowWin32(HINSTANCE hInstance, ATOM aWndClass, COSWindowWin32* pParentWnd = nullptr);
	virtual ~COSWindowWin32();

	virtual void			SetTitle(const char* pTitle) override;
	virtual void			SetIcon(const char* pIconName) override;
	virtual bool			SetCursor(const char* pCursorName) override;
	virtual bool			SetRect(const Data::CRect& NewRect, bool FullscreenMode = false) override;
	virtual bool			SetInputFocus() override;

	virtual bool			Show() override;
	virtual bool			Hide() override;
	virtual void			Close() override;

	virtual bool			IsValid() const override { return !!hWnd; }
	virtual bool			HasInputFocus() const override;
	virtual COSWindow*		GetParent() const override { return pParent; }
	virtual CString			GetTitle() const override;
	virtual bool			GetCursorPosition(IPTR& OutX, IPTR& OutY) const override;

	void					Minimize();
	void					Restore();
	bool					HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LONG& OutResult); // Mainly for internal use

	bool					GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const;
	bool					GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	bool					SetTopmost(bool Topmost);

	HWND					GetHWND() const { return hWnd; }
	LONG					GetWin32Style() const;
};

inline bool COSWindowWin32::GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const
{
	RECT r;
	if (!hWnd || !::GetClientRect(hWnd, &r)) FAIL;
	XAbs = (int)(XRel * std::max<LONG>(r.right - r.left, 1));
	YAbs = (int)(YRel * std::max<LONG>(r.bottom - r.top, 1));
	OK;
}
//---------------------------------------------------------------------

inline bool COSWindowWin32::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	RECT r;
	if (!hWnd || !::GetClientRect(hWnd, &r)) FAIL;
	XRel = XAbs / float(std::max<LONG>(r.right - r.left, 1));
	YRel = YAbs / float(std::max<LONG>(r.bottom - r.top, 1));
	OK;
}
//---------------------------------------------------------------------

}

#endif
