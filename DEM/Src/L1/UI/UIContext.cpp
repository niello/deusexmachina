#include "UIContext.h"

#include <CEGUI/RenderTarget.h>
#include <Events/EventDispatcher.h>
#include <System/Events/OSInput.h>
#include <Input/Events/KeyDown.h>
#include <Input/Events/KeyUp.h>
#include <Input/Events/CharInput.h>
#include <Input/Events/MouseMove.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseWheel.h>

namespace UI
{

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

bool CUIContext::Render(float Left, float Top, float Right, float Bottom)
{
	if (!pCtx) FAIL;
	CEGUI::Renderer* pRenderer = CEGUI::System::getSingleton().getRenderer();
	if (!pRenderer) FAIL;

	CEGUI::Rectf ViewportArea(Left, Top, Right, Bottom);
	if (pCtx->getRenderTarget().getArea() != ViewportArea)
		pCtx->getRenderTarget().setArea(ViewportArea);

	pRenderer->beginRendering();
	pCtx->draw();
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
	pRootWindow = pWindow;
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

//???does work in all cases?
bool CUIContext::IsMouseOverGUI() const
{
	if (!pCtx) FAIL;
	CEGUI::Window* pMouseWnd = pCtx->getWindowContainingMouse();
	return pMouseWnd && pMouseWnd != pCtx->getRootWindow();
}
//---------------------------------------------------------------------

bool CUIContext::OnOSWindowInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::OSInput& Ev = (const Event::OSInput&)Event;

	switch (Ev.Type)
	{
		case Event::OSInput::KeyDown:
			return pCtx->injectKeyDown((CEGUI::Key::Scan)Ev.KeyCode);

		case Event::OSInput::KeyUp:
			return pCtx->injectKeyUp((CEGUI::Key::Scan)Ev.KeyCode);

		case Event::OSInput::CharInput:
			return pCtx->injectChar(Ev.Char);

		case Event::OSInput::MouseMove:
			return pCtx->injectMousePosition((float)Ev.MouseInfo.x, (float)Ev.MouseInfo.y);

		case Event::OSInput::MouseDown:
			return pCtx->injectMouseButtonDown((CEGUI::MouseButton)Ev.MouseInfo.Button);

		case Event::OSInput::MouseUp:
			return pCtx->injectMouseButtonUp((CEGUI::MouseButton)Ev.MouseInfo.Button);

		case Event::OSInput::MouseWheel:
			return pCtx->injectMouseWheelChange((float)Ev.WheelDelta);
	}

	FAIL;
}
//---------------------------------------------------------------------

}
