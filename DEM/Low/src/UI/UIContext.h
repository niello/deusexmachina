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
	bool					WasPassThroughEnabledInRoot = false;

	void				SetRootWindow(CUIWindow* pWindow);

	bool				OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	DECLARE_2_EVENTS_HANDLER(OnToggleFullscreen, OSWindowResized, OnViewportSizeChanged);

public:

	//???pass RT params and create CEGUI context inside?
	CUIContext(CEGUI::GUIContext* pContext, DEM::Sys::COSWindow* pHostWindow);
	virtual ~CUIContext() override;

	// Pass absolute viewport coordinates here
	bool				Render(EDrawMode Mode, float Left, float Top, float Right, float Bottom);

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
	void				SetDefaultMouseCursor(const char* pImageName); //???also add SetMouseCursor?
	bool				GetCursorPosition(float& X, float& Y) const;
	bool				GetCursorPositionRel(float& X, float& Y) const;
	bool				IsMouseOverGUI() const;
};

typedef Ptr<CUIContext> PUIContext;

}
