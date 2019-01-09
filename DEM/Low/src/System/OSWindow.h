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

	enum
	{
		Wnd_Opened = 0x01,
		Wnd_Minimized = 0x02,
		Wnd_Topmost = 0x04,
		Wnd_Fullscreen = 0x08
	};

	CString			WindowTitle;
	CString			IconName;

	Data::CRect		Rect;					// Client rect
	Data::CFlags	Flags;

public:

	virtual bool			SetRect(const Data::CRect& NewRect, bool FullscreenMode = false) = 0;
	virtual bool			SetInputFocus() = 0;

	virtual COSWindow*		GetParent() const = 0;

	const char*				GetTitle() const { return WindowTitle; }
	const char*				GetIcon() const { return IconName; }
	const Data::CRect&		GetRect() const { return Rect; }
	unsigned int			GetWidth() const { return Rect.W; }
	unsigned int			GetHeight() const { return Rect.H; }
	bool					IsChild() const { return !!GetParent(); }
	bool					IsOpen() const { return Flags.Is(Wnd_Opened); }
	bool					IsMinimized() const { return Flags.Is(Wnd_Minimized); }
	bool					IsTopmost() const { return Flags.Is(Wnd_Topmost); }
	bool					IsFullscreen() const { return Flags.Is(Wnd_Fullscreen); }
};

}}
