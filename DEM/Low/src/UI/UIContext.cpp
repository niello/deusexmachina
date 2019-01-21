#include "UIContext.h"

#include <CEGUI/RenderTarget.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <Events/EventDispatcher.h>
#include <Events/Subscription.h>
#include <Input/InputEvents.h>
#include <System/Events/OSInput.h>

namespace UI
{
CUIContext::CUIContext() {}

CUIContext::~CUIContext()
{
	if (pCtx && pCtx != &CEGUI::System::getSingleton().getDefaultGUIContext())
		CEGUI::System::getSingleton().destroyGUIContext(*pCtx);
}
//---------------------------------------------------------------------

void CUIContext::Init(CEGUI::GUIContext* pContext)
{
	pCtx = pContext;
	if (pCtx) pCtx->setMouseClickEventGenerationEnabled(true);
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

	if (Mode & DrawMode_Opaque)
	{
		((CEGUI::CDEMRenderer*)pRenderer)->setOpaqueMode(true);
		pCtx->draw(DrawModeFlagWindowOpaque);
	}

	if (Mode & DrawMode_Transparent)
	{
		((CEGUI::CDEMRenderer*)pRenderer)->setOpaqueMode(false);
		pCtx->draw(CEGUI::DrawModeFlagWindowRegular | CEGUI::DrawModeFlagMouseCursor);
	}

	pRenderer->endRendering();

	OK;
}
//---------------------------------------------------------------------

bool CUIContext::SubscribeOnInput(Events::CEventDispatcher* pDispatcher, U16 Priority)
{
	if (!pDispatcher || !pCtx) FAIL;

	// CEGUI 0.7.x:
	// For correct bool return from injects app should have a fullscreen DefaultWindow
	// as layout root with the MousePassThroughEnabled property set to true.
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, OSInput, CUIContext, OnOSWindowInput, Priority);
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, AxisMove, CUIContext, OnAxisMove, Priority);
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, ButtonDown, CUIContext, OnButtonDown, Priority);
	DISP_SUBSCRIBE_NEVENT_PRIORITY(pDispatcher, ButtonUp, CUIContext, OnButtonUp, Priority);

	OK;
}
//---------------------------------------------------------------------

void CUIContext::UnsubscribeFromInput()
{
	UNSUBSCRIBE_EVENT(OSInput);
}
//---------------------------------------------------------------------

void CUIContext::SetRootWindow(CUIWindow* pWindow)
{
	RootWindow = pWindow;
	if (pCtx)
	{
		pCtx->setRootWindow(pWindow ? pWindow->GetWnd() : NULL);
		pCtx->updateWindowContainingMouse();
	}
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
	if (pCtx) pCtx->getMouseCursor().show();
}
//---------------------------------------------------------------------

void CUIContext::HideMouseCursor()
{
	if (pCtx) pCtx->getMouseCursor().hide();
}
//---------------------------------------------------------------------

void CUIContext::SetDefaultMouseCursor(const char* pImageName)
{
	if (pCtx) pCtx->getMouseCursor().setDefaultImage(pImageName);
}
//---------------------------------------------------------------------

bool CUIContext::GetCursorPosition(float& X, float& Y) const
{
	if (pCtx)
	{
		CEGUI::Vector2f CursorPos = pCtx->getMouseCursor().getPosition();
		X = CursorPos.d_x;
		Y = CursorPos.d_y;
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
		CEGUI::Vector2f CursorPos = pCtx->getMouseCursor().getPosition();
		X = CursorPos.d_x / CtxSize.d_width;
		Y = CursorPos.d_y / CtxSize.d_height;
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::IsMouseOverGUI() const
{
	if (!pCtx) FAIL;
	CEGUI::Window* pMouseWnd = pCtx->getWindowContainingMouse();
	return pMouseWnd && pMouseWnd != pCtx->getRootWindow();
}
//---------------------------------------------------------------------

// NB: DEM keyboard key codes match CEGUI scancodes, so no additional mapping is required
bool CUIContext::OnOSWindowInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::OSInput& Ev = (const Event::OSInput&)Event;

	switch (Ev.Type)
	{
		case Event::OSInput::KeyDown:
		{
			if (pCtx->injectKeyDown((CEGUI::Key::Scan)Ev.KeyboardInfo.ScanCode)) OK;
			return Ev.KeyboardInfo.Char != 0 && pCtx->injectChar(Ev.KeyboardInfo.Char);
		}

		case Event::OSInput::KeyUp:
			return pCtx->injectKeyUp((CEGUI::Key::Scan)Ev.KeyboardInfo.ScanCode);

		case Event::OSInput::MouseMove:
			return pCtx->injectMousePosition((float)Ev.MouseInfo.x, (float)Ev.MouseInfo.y);

		case Event::OSInput::MouseDown:
		{
			CEGUI::MouseButton Button;
			switch (Ev.MouseInfo.Button)
			{
				case 0:		Button = CEGUI::LeftButton; break;
				case 1:		Button = CEGUI::RightButton; break;
				case 2:		Button = CEGUI::MiddleButton; break;
				case 3:		Button = CEGUI::X1Button; break;
				case 4:		Button = CEGUI::X2Button; break;
				default:	FAIL;
			}
			return pCtx->injectMouseButtonDown(Button);
		}

		case Event::OSInput::MouseUp:
		{
			CEGUI::MouseButton Button;
			switch (Ev.MouseInfo.Button)
			{
				case 0:		Button = CEGUI::LeftButton; break;
				case 1:		Button = CEGUI::RightButton; break;
				case 2:		Button = CEGUI::MiddleButton; break;
				case 3:		Button = CEGUI::X1Button; break;
				case 4:		Button = CEGUI::X2Button; break;
				default:	FAIL;
			}

			//FIXME: as CEGUI currently marks all mouse button events as handled, we have to do the opposite here
			//to track mouse up events properly. Mouse down events are handled without this hack.
			//return pCtx->injectMouseButtonUp(Button);
			pCtx->injectMouseButtonUp(Button);
			FAIL;
		}

		case Event::OSInput::MouseWheelVertical:
		case Event::OSInput::MouseWheelHorizontal:
			return pCtx->injectMouseWheelChange((float)Ev.WheelDelta);
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnAxisMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnButtonDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FAIL;
}
//---------------------------------------------------------------------

bool CUIContext::OnButtonUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	FAIL;
}
//---------------------------------------------------------------------

}
