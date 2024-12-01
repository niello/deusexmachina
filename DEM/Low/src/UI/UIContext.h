#pragma once
#include <UI/UIWindow.h>
#include <Events/EventsFwd.h>

// UI context is a set of elements rendered to one target and receiving the same input.
// Each engine window or viewport may have it's own UI context independent of each other.

namespace DEM::Sys
{
using POSWindow = Ptr<class COSWindow>;
}

namespace UI
{
using PUIWindow = Ptr<class CUIWindow>;
using PUIContext = Ptr<class CUIContext>;

class CUIContext: public Core::CObject
{
private:

	std::vector<PUIWindow>    RootWindows; // Stack, top is back
	DEM::Sys::POSWindow       OSWindow;
	CEGUI::GUIContext*        pCtx = nullptr;

	std::map<const ::Events::CEventDispatcher*, std::vector<::Events::PSub>> _InputSubs;

	void				SetRootWindow(CUIWindow* pWindow);
	void                DetachRootWindow(PUIWindow&& Window);

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
	void				UnsubscribeFromInput(Events::CEventDispatcher* pDispatcher = nullptr);

	void				PushRootWindow(CUIWindow* pWindow);
	PUIWindow			PopRootWindow();
	void                RemoveRootWindow(CUIWindow* pWindow);
	void				ClearRootWindowStack();
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

}
