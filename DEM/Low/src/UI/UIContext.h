#pragma once
#include <UI/UIWindow.h>
#include <Events/EventsFwd.h>
#include <Data/Array.h>

// UI context is a set of elements rendered to one target and receiving the same input.
// Each engine window or viewport may have it's own UI context independent of each other.

namespace DEM::Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}

namespace UI
{
typedef Ptr<class CUIWindow> PUIWindow;

class CUIContext: public Core::CObject
{
private:

	std::vector<PUIWindow>	RootWindows; // Stack, top is back

	DEM::Sys::POSWindow		OSWindow;
	CArray<Events::PSub>	InputSubs;
	CEGUI::GUIContext*		pCtx = nullptr;
	CEGUI::InputAggregator*	pInput = nullptr;
	bool					InputEventsOnKeyUp = true;

	void				SetRootWindow(CUIWindow* pWindow);

	bool				OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	DECLARE_EVENT_HANDLER(OnFocusSet, OnFocusSet);
	DECLARE_2_EVENTS_HANDLER(OnToggleFullscreen, OSWindowResized, OnViewportSizeChanged);

public:

	CUIContext(float Width, float Height, DEM::Sys::COSWindow* pHostWindow);
	virtual ~CUIContext() override;

	// Pass absolute viewport coordinates here
	void                Update(float dt);

	bool				SubscribeOnInput(Events::CEventDispatcher* pDispatcher, U16 Priority);
	void				UnsubscribeFromInput(Events::CEventDispatcher* pDispatcher);
	void				UnsubscribeFromInput();

	void				ClearWindowStack();
	void				PushRootWindow(CUIWindow* pWindow);
	PUIWindow			PopRootWindow();
	CUIWindow*			GetRootWindow() const;

	void				ShowGUI();
	void				HideGUI();
	void				ShowMouseCursor();
	void				HideMouseCursor();
	void				SetMouseCursor(const char* pImageName);
	void				SetDefaultMouseCursor(const char* pImageName);
	bool				GetCursorPosition(float& X, float& Y) const;
	bool				GetCursorPositionRel(float& X, float& Y) const;
	bool				IsMouseOverGUI() const;

	CEGUI::GUIContext*	GetCEGUIContext() const { return pCtx; }
};

typedef Ptr<CUIContext> PUIContext;

}
