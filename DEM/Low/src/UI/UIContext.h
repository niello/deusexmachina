#pragma once
#include <UI/UIWindow.h>
#include <Events/EventsFwd.h>

// UI context is a set of elements rendered to one target and receiving the same input.
// Each engine window or viewport may have it's own UI context independent of each other.

namespace DEM { namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}}

namespace UI
{
typedef Ptr<class CUIWindow> PUIWindow;

class CUIContext: public Core::CObject
{
private:

	std::vector<PUIWindow>	RootWindows; // Stack, top is back

	DEM::Sys::POSWindow		OSWindow;
	CEGUI::GUIContext*		pCtx = nullptr;
	CEGUI::InputAggregator*	pInput= nullptr;
	bool					WasPassThroughEnabledInRoot = false;

	void				SetRootWindow(CUIWindow* pWindow);

	DECLARE_EVENT_HANDLER(AxisMove, OnAxisMove);
	DECLARE_EVENT_HANDLER(ButtonDown, OnButtonDown);
	DECLARE_EVENT_HANDLER(ButtonUp, OnButtonUp);
	DECLARE_EVENT_HANDLER(TextInput, OnTextInput);

	DECLARE_2_EVENTS_HANDLER(OnToggleFullscreen, OnSizeChanged, OnViewportSizeChanged);

public:

	//!!!pass OS window and render target params, either intermediate RT or swap chain index! or unify?
	CUIContext();
	~CUIContext();

	void				Init(CEGUI::GUIContext* pContext, DEM::Sys::COSWindow* pHostWindow);
	//CEGUI::GUIContext*	GetCEGUIContext() const { return pCtx; }

	// Pass absolute viewport coordinates here
	bool				Render(EDrawMode Mode, float Left, float Top, float Right, float Bottom);

	bool				SubscribeOnInput(Events::CEventDispatcher* pDispatcher, U16 Priority);
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
