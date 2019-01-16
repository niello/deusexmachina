#pragma once
#if DEM_PLATFORM_WIN32
#include <System/OSWindow.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Win32 operating system window implementation

namespace DEM { namespace Sys
{
typedef Ptr<class COSWindowWin32> POSWindowWin32;

class COSWindowWin32 : public COSWindow
{
	__DeclareClassNoFactory;

protected:

	COSWindowWin32*	pParent = nullptr;
	HWND			hWnd = 0;

public:

	COSWindowWin32(HINSTANCE hInstance, ATOM aWndClass, COSWindowWin32* pParentWnd = nullptr);
	virtual ~COSWindowWin32();

	virtual void			SetTitle(const char* pTitle) override;
	virtual void			SetIcon(const char* pIconName) override;
	virtual bool			SetRect(const Data::CRect& NewRect, bool FullscreenMode = false) override;
	virtual bool			SetInputFocus() override;

	virtual bool			Show() override;
	virtual bool			Hide() override;
	virtual void			Close() override;

	virtual bool			IsValid() const override { return !!hWnd; }
	virtual COSWindow*		GetParent() const override { return pParent; }
	virtual CString			GetTitle() const override;

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
	XAbs = (int)(XRel * n_max(r.right - r.left, 1));
	YAbs = (int)(YRel * n_max(r.bottom - r.top, 1));
	OK;
}
//---------------------------------------------------------------------

inline bool COSWindowWin32::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	RECT r;
	if (!hWnd || !::GetClientRect(hWnd, &r)) FAIL;
	XRel = XAbs / float(n_max(r.right - r.left, 1));
	YRel = YAbs / float(n_max(r.bottom - r.top, 1));
	OK;
}
//---------------------------------------------------------------------

}}

#endif
