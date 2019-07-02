#include "UIContext.h"

#include <UI/CEGUI/DEMRenderer.h>
#include <Events/EventDispatcher.h>
#include <Events/Subscription.h>
#include <Input/InputEvents.h>
#include <System/OSWindow.h>

#include <CEGUI/RenderTarget.h>
#include <CEGUI/InputAggregator.h>

namespace UI
{
CUIContext::CUIContext() {}

CUIContext::~CUIContext()
{
	SAFE_DELETE(pInput);

	if (pCtx)
		CEGUI::System::getSingleton().destroyGUIContext(*pCtx);
}
//---------------------------------------------------------------------

void CUIContext::Init(CEGUI::GUIContext* pContext, DEM::Sys::COSWindow* pHostWindow)
{
	UNSUBSCRIBE_EVENT(OnToggleFullscreen);
	UNSUBSCRIBE_EVENT(OnSizeChanged);

	SAFE_DELETE(pInput);

	OSWindow = pHostWindow;
	pCtx = pContext;

	if (!pContext) return;

	pInput = n_new(CEGUI::InputAggregator(pCtx));
	pInput->initialise();
	pInput->setMouseClickEventGenerationEnabled(true);

	if (pHostWindow)
	{
		DISP_SUBSCRIBE_PEVENT(pHostWindow, OnToggleFullscreen, CUIContext, OnViewportSizeChanged);
		DISP_SUBSCRIBE_PEVENT(pHostWindow, OnSizeChanged, CUIContext, OnViewportSizeChanged);
	}
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

	pRenderer->beginRendering();

	// TODO: CEGUI fix

	if (Mode & DrawMode_Opaque)
	{
//		((CEGUI::CDEMRenderer*)pRenderer)->setOpaqueMode(true);
//		pCtx->draw(DrawModeFlagWindowOpaque);
	}

	if (Mode & DrawMode_Transparent)
	{
//		((CEGUI::CDEMRenderer*)pRenderer)->setOpaqueMode(false);
		pCtx->draw(/*CEGUI::DrawModeFlagWindowRegular | CEGUI::DrawModeFlagMouseCursor*/);
	}

	pRenderer->endRendering();

	OK;
}
//---------------------------------------------------------------------

bool CUIContext::SubscribeOnInput(Events::CEventDispatcher* pDispatcher, U16 Priority)
{
	if (!pDispatcher || !pCtx || !pInput) FAIL;

	// CEGUI 0.7.x:
	// For correct bool return from injects app should have a fullscreen DefaultWindow
	// as layout root with the MousePassThroughEnabled property set to true.
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, AxisMove, CUIContext, OnAxisMove, Priority);
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, ButtonDown, CUIContext, OnButtonDown, Priority);
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, ButtonUp, CUIContext, OnButtonUp, Priority);
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, TextInput, CUIContext, OnTextInput, Priority);

	OK;
}
//---------------------------------------------------------------------

void CUIContext::UnsubscribeFromInput()
{
	UNSUBSCRIBE_EVENT(AxisMove);
	UNSUBSCRIBE_EVENT(ButtonDown);
	UNSUBSCRIBE_EVENT(ButtonUp);
	UNSUBSCRIBE_EVENT(TextInput);
}
//---------------------------------------------------------------------

void CUIContext::SetRootWindow(CUIWindow* pWindow)
{
	if (!pCtx) return;

	//???configure in data and never touch in code?
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

	pCtx->setInputCaptureWindow(nullptr); // FIXME: to CEGUI setRootWindow()!
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
	const Event::AxisMove& Ev = static_cast<const Event::AxisMove&>(Event);

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
	const Event::ButtonDown& Ev = static_cast<const Event::ButtonDown&>(Event);

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
		return pInput->injectKeyDown((CEGUI::Key::Scan)Ev.Code);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::ButtonUp& Ev = static_cast<const Event::ButtonUp&>(Event);

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
		return pInput->injectKeyUp((CEGUI::Key::Scan)Ev.Code);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnTextInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::TextInput& Ev = static_cast<const Event::TextInput&>(Event);

	bool Handled = false;
	for (char Char : Ev.Text)
		Handled |= pInput->injectChar(Char);
	return Handled;
}
//---------------------------------------------------------------------

bool CUIContext::OnViewportSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	if (pCtx && OSWindow)
	{
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
	}
	FAIL;
}
//---------------------------------------------------------------------

}
