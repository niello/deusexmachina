#pragma once
#include <Events/EventDispatcher.h>
#include <Data/Regions.h>

// Operating system window base

namespace DEM { namespace Sys
{
typedef Ptr<class COSWindow> POSWindow;

class COSWindow : public Events::CEventDispatcher, public ::Core::CObject
{
protected:

	//!!!make booleans!
	enum
	{
		Wnd_Minimized = 0x02,
		Wnd_Topmost = 0x04,
		Wnd_Fullscreen = 0x08
	};

	CString			IconName;

	Data::CRect		Rect;					// Client rect
	Data::CFlags	Flags;

public:

	virtual ~COSWindow() {}

	virtual void			SetTitle(const char* pTitle) = 0;
	virtual void			SetIcon(const char* pIconName) = 0;
	virtual bool			SetCursor(const char* pCursorName) = 0;
	virtual bool			SetRect(const Data::CRect& NewRect, bool FullscreenMode = false) = 0;
	virtual bool			SetInputFocus() = 0;

	virtual bool			Show() = 0;
	virtual bool			Hide() = 0;
	virtual void			Close() = 0;

	virtual bool			IsValid() const = 0;
	virtual bool			HasInputFocus() const = 0;
	virtual COSWindow*		GetParent() const = 0;
	virtual CString			GetTitle() const = 0;
	virtual bool			GetCursorPosition(IPTR& OutX, IPTR& OutY) const = 0;

	bool					IsChild() const { return !!GetParent(); }

	//???what is virtual, what is not? make it a pure interface?
	const char*				GetIcon() const { return IconName; }
	const Data::CRect&		GetRect() const { return Rect; }
	unsigned int			GetWidth() const { return Rect.W; }
	unsigned int			GetHeight() const { return Rect.H; }
	bool					IsMinimized() const { return Flags.Is(Wnd_Minimized); }
	bool					IsTopmost() const { return Flags.Is(Wnd_Topmost); }
	bool					IsFullscreen() const { return Flags.Is(Wnd_Fullscreen); }
};

}}
