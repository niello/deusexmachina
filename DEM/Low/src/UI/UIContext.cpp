#include "UIContext.h"

#include <UI/CEGUI/DEMRenderer.h>
#include <Events/EventDispatcher.h>
#include <Events/Subscription.h>
#include <Input/InputEvents.h>
#include <System/OSWindow.h>
#include <System/SystemEvents.h>

#include <CEGUI/RenderTarget.h>
#include <CEGUI/InputAggregator.h>

namespace UI
{

CUIContext::CUIContext(CEGUI::GUIContext* pContext, DEM::Sys::COSWindow* pHostWindow)
	: pCtx(pContext)
	, OSWindow(pHostWindow)
{
	if (!pContext) return;

	pInput = n_new(CEGUI::InputAggregator(pCtx));
	pInput->initialise(InputEventsOnKeyUp);
	pInput->setMouseClickEventGenerationEnabled(true);

	if (pHostWindow)
	{
		// FIXME: CEGUI doesn't calculate proper font size at the start
		// FIXME: must be per-context, not global!
		const Data::CRect& WndRect = pHostWindow->GetRect();
		const CEGUI::Sizef RectSize(static_cast<float>(WndRect.W), static_cast<float>(WndRect.H));
		CEGUI::System::getSingleton().notifyDisplaySizeChanged(RectSize);

		DISP_SUBSCRIBE_PEVENT(pHostWindow, OnToggleFullscreen, CUIContext, OnViewportSizeChanged);
		DISP_SUBSCRIBE_NEVENT(pHostWindow, OSWindowResized, CUIContext, OnViewportSizeChanged);
	}
}
//---------------------------------------------------------------------

CUIContext::~CUIContext()
{
	SAFE_DELETE(pInput);

	if (pCtx)
		CEGUI::System::getSingleton().destroyGUIContext(*pCtx);
}
//---------------------------------------------------------------------

bool CUIContext::Render(EDrawMode Mode, float Left, float Top, float Right, float Bottom)
{
	if (!pCtx) FAIL;
	CEGUI::Renderer* pRenderer = CEGUI::System::getSingleton().getRenderer();
	if (!pRenderer) FAIL;
	if (!Mode) OK;

	CEGUI::Rectf ViewportArea(Left, Top, Right, Bottom);
	if (pCtx->getRenderTarget().getArea() != ViewportArea)
		pCtx->getRenderTarget().setArea(ViewportArea);

	// FIXME: CEGUI drawMode concept doesn't fully fit into requirements of
	// opaque/transparent separation, so draw all in a transparent phase for now.
	if (!(Mode & DrawMode_Transparent)) OK;
		
	pRenderer->beginRendering();

	//if (Mode & DrawMode_Opaque)
	//	pCtx->draw(DrawModeFlagWindowOpaque);

	//if (Mode & DrawMode_Transparent)
	//	pCtx->draw(CEGUI::DrawModeFlagWindowRegular | CEGUI::DrawModeFlagMouseCursor);
	pCtx->draw();

	pRenderer->endRendering();

	OK;
}
//---------------------------------------------------------------------

bool CUIContext::SubscribeOnInput(Events::CEventDispatcher* pDispatcher, U16 Priority)
{
	if (!pDispatcher || !pCtx || !pInput) FAIL;

	Events::PSub NewSub;
	pDispatcher->Subscribe(&Event::AxisMove::RTTI, this, &CUIContext::OnAxisMove, &NewSub);
	InputSubs.Add(NewSub);
	pDispatcher->Subscribe(&Event::ButtonDown::RTTI, this, &CUIContext::OnButtonDown, &NewSub);
	InputSubs.Add(NewSub);
	pDispatcher->Subscribe(&Event::ButtonUp::RTTI, this, &CUIContext::OnButtonUp, &NewSub);
	InputSubs.Add(NewSub);
	pDispatcher->Subscribe(&Event::TextInput::RTTI, this, &CUIContext::OnTextInput, &NewSub);
	InputSubs.Add(NewSub);

	OK;
}
//---------------------------------------------------------------------

void CUIContext::UnsubscribeFromInput(Events::CEventDispatcher* pDispatcher)
{
	if (!pDispatcher) return;

	for (UPTR i = 0; i < InputSubs.GetCount(); )
	{
		if (InputSubs[i]->GetDispatcher() == pDispatcher)
			InputSubs.RemoveAt(i);
		else ++i;
	}
}
//---------------------------------------------------------------------

void CUIContext::UnsubscribeFromInput()
{
	InputSubs.Clear();
}
//---------------------------------------------------------------------

void CUIContext::SetRootWindow(CUIWindow* pWindow)
{
	if (!pCtx) return;

	//???configure in data and never touch in code?
	//May need input-opaque screens, like modal windows
	if (!WasPassThroughEnabledInRoot)
	{
		CEGUI::Window* pPrevRoot = pCtx->getRootWindow();
		if (pPrevRoot) pPrevRoot->setCursorPassThroughEnabled(false);
	}

	if (pWindow && pWindow->GetWnd())
	{
		WasPassThroughEnabledInRoot = pWindow->GetWnd()->isCursorPassThroughEnabled();
		pWindow->GetWnd()->setCursorPassThroughEnabled(true);
		pCtx->setRootWindow(pWindow->GetWnd());
	}
	else
	{
		pCtx->setRootWindow(nullptr);
	}

	//???!!!FIXME: to CEGUI?
	pCtx->updateWindowContainingCursor();
}
//---------------------------------------------------------------------

void CUIContext::ClearWindowStack()
{
	SetRootWindow(nullptr);
	RootWindows.clear();
}
//---------------------------------------------------------------------

void CUIContext::PushRootWindow(CUIWindow* pWindow)
{
	if (!pWindow) return;

	if (!RootWindows.empty() && RootWindows.back() == pWindow) return;

	RootWindows.push_back(pWindow);
	SetRootWindow(pWindow);
}
//---------------------------------------------------------------------

PUIWindow CUIContext::PopRootWindow()
{
	if (RootWindows.empty()) return nullptr;

	PUIWindow CurrWnd = GetRootWindow();
	RootWindows.pop_back();
	SetRootWindow(GetRootWindow());

	return CurrWnd;
}
//---------------------------------------------------------------------

CUIWindow* CUIContext::GetRootWindow() const
{
	return RootWindows.empty() ? nullptr : RootWindows.back().Get();
}
//---------------------------------------------------------------------

void CUIContext::ShowGUI()
{
	if (!pCtx) return;
	CEGUI::Window* pRoot = pCtx->getRootWindow();
	if (pRoot) pRoot->setVisible(true);
}
//---------------------------------------------------------------------

void CUIContext::HideGUI()
{
	if (!pCtx) return;
	CEGUI::Window* pRoot = pCtx->getRootWindow();
	if (pRoot) pRoot->setVisible(false);
}
//---------------------------------------------------------------------

void CUIContext::ShowMouseCursor()
{
	if (pCtx) pCtx->getCursor().show();
}
//---------------------------------------------------------------------

void CUIContext::HideMouseCursor()
{
	if (pCtx) pCtx->getCursor().hide();
}
//---------------------------------------------------------------------

void CUIContext::SetDefaultMouseCursor(const char* pImageName)
{
	if (pCtx) pCtx->getCursor().setDefaultImage(pImageName);
}
//---------------------------------------------------------------------

bool CUIContext::GetCursorPosition(float& X, float& Y) const
{
	if (pCtx)
	{
		glm::vec2 CursorPos = pCtx->getCursor().getPosition();
		X = CursorPos.x;
		Y = CursorPos.y;
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::GetCursorPositionRel(float& X, float& Y) const
{
	if (pCtx)
	{
		CEGUI::Sizef CtxSize = pCtx->getRootWindow()->getPixelSize();
		glm::vec2 CursorPos = pCtx->getCursor().getPosition();
		X = CursorPos.x / CtxSize.d_width;
		Y = CursorPos.y / CtxSize.d_height;
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::IsMouseOverGUI() const
{
	if (!pCtx) FAIL;
	CEGUI::Window* pMouseWnd = pCtx->getWindowContainingCursor();
	return pMouseWnd && pMouseWnd != pCtx->getRootWindow();
}
//---------------------------------------------------------------------

bool CUIContext::OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::AxisMove& Ev = static_cast<const Event::AxisMove&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnAxisMove(pDispatcher, Ev)) OK;

	if (Ev.Device->GetType() == Input::Device_Mouse)
	{
		switch (Ev.Code)
		{
			case 0:
			case 1:
			{
				IPTR X, Y;
				return OSWindow && OSWindow->GetCursorPosition(X, Y) &&
					pInput->injectMousePosition((float)X, (float)Y);
			}

			case 2:
			case 3:
				return pInput->injectMouseWheelChange(Ev.Amount);
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::ButtonDown& Ev = static_cast<const Event::ButtonDown&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnButtonDown(pDispatcher, Ev)) OK;

	if (Ev.Device->GetType() == Input::Device_Mouse)
	{
		switch (Ev.Code)
		{
			case 0:		return pInput->injectMouseButtonDown(CEGUI::MouseButton::Left);
			case 1:		return pInput->injectMouseButtonDown(CEGUI::MouseButton::Right);
			case 2:		return pInput->injectMouseButtonDown(CEGUI::MouseButton::Middle);
			case 3:		return pInput->injectMouseButtonDown(CEGUI::MouseButton::X1);
			case 4:		return pInput->injectMouseButtonDown(CEGUI::MouseButton::X2);
			default:	FAIL;
		}
	}
	else if (Ev.Device->GetType() == Input::Device_Keyboard)
	{
		// NB: DEM keyboard key codes match CEGUI scancodes, so no additional mapping is required
		// FIXME CEGUI: input aggregator returns true despite it didn't generate an event when d_handleInKeyUp
		const bool Handled = pInput->injectKeyDown((CEGUI::Key::Scan)Ev.Code);
		return !InputEventsOnKeyUp && Handled;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::ButtonUp& Ev = static_cast<const Event::ButtonUp&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnButtonUp(pDispatcher, Ev)) OK;

	if (Ev.Device->GetType() == Input::Device_Mouse)
	{
		switch (Ev.Code)
		{
			case 0:		return pInput->injectMouseButtonUp(CEGUI::MouseButton::Left);
			case 1:		return pInput->injectMouseButtonUp(CEGUI::MouseButton::Right);
			case 2:		return pInput->injectMouseButtonUp(CEGUI::MouseButton::Middle);
			case 3:		return pInput->injectMouseButtonUp(CEGUI::MouseButton::X1);
			case 4:		return pInput->injectMouseButtonUp(CEGUI::MouseButton::X2);
			default:	FAIL;
		}
	}
	else if (Ev.Device->GetType() == Input::Device_Keyboard)
	{
		// NB: DEM keyboard key codes match CEGUI scancodes, so no additional mapping is required
		// FIXME CEGUI: input aggregator returns true despite it didn't generate an event when not d_handleInKeyUp
		const bool Handled = pInput->injectKeyUp((CEGUI::Key::Scan)Ev.Code);
		return InputEventsOnKeyUp && Handled;
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (!OSWindow || !OSWindow->HasInputFocus()) FAIL;

	const Event::TextInput& Ev = static_cast<const Event::TextInput&>(Event);

	if (!RootWindows.empty() && RootWindows.back()->OnTextInput(pDispatcher, Ev)) OK;

	bool Handled = false;
	for (char Char : Ev.Text)
		Handled |= pInput->injectChar(Char);
	return Handled;
}
//---------------------------------------------------------------------

bool CUIContext::OnViewportSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const auto& Ev = static_cast<const Event::OSWindowResized&>(Event);
	if (Ev.ManualResizingInProgress) FAIL;

	if (!pCtx || !OSWindow) FAIL;

	const Data::CRect& WndRect = OSWindow->GetRect();
	const CEGUI::Sizef RectSize(static_cast<float>(WndRect.W), static_cast<float>(WndRect.H));

	// FIXME: fonts & images scale based on it, so we must call it for now, but the display size
	//        doesn't change here actually, it is a render target size change, context-local!
	CEGUI::System::getSingleton().notifyDisplaySizeChanged(RectSize);

	auto& RT = pCtx->getRenderTarget();
	CEGUI::Rectf GUIArea(RT.getArea());
	GUIArea.setSize(RectSize);
	RT.setArea(GUIArea);

	IPTR X, Y;
	if (OSWindow->GetCursorPosition(X, Y))
		pInput->injectMousePosition(static_cast<float>(X), static_cast<float>(Y));

	OK;
}
//---------------------------------------------------------------------

}
