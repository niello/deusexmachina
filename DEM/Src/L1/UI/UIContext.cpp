#include "UIContext.h"

//#include <Input/InputServer.h>
//#include <Input/Events/KeyDown.h>
//#include <Input/Events/KeyUp.h>
//#include <Input/Events/CharInput.h>
//#include <Input/Events/MouseMove.h>
//#include <Input/Events/MouseBtnDown.h>
//#include <Input/Events/MouseBtnUp.h>
//#include <Input/Events/MouseWheel.h>

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
}
//---------------------------------------------------------------------

bool CUIContext::Render()
{
	if (!pCtx) FAIL;
	CEGUI::Renderer* pRenderer = CEGUI::System::getSingleton().getRenderer();
	if (!pRenderer) FAIL;
	pRenderer->beginRendering();
	pCtx->draw();
	pRenderer->endRendering();
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

	//// CEGUI 0.7.x:
	//// For correct bool return from injects app should have a fullscreen DefaultWindow
	//// as layout root with the MousePassThroughEnabled property set to true.
	//SUBSCRIBE_INPUT_EVENT(KeyDown, CUIServer, OnKeyDown, Input::InputPriority_UI);
	//SUBSCRIBE_INPUT_EVENT(KeyUp, CUIServer, OnKeyUp, Input::InputPriority_UI);
	//SUBSCRIBE_INPUT_EVENT(CharInput, CUIServer, OnCharInput, Input::InputPriority_UI);
	//SUBSCRIBE_INPUT_EVENT(MouseMove, CUIServer, OnMouseMove, Input::InputPriority_UI);
	//SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CUIServer, OnMouseBtnDown, Input::InputPriority_UI);
	//SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CUIServer, OnMouseBtnUp, Input::InputPriority_UI);
	//SUBSCRIBE_INPUT_EVENT(MouseWheel, CUIServer, OnMouseWheel, Input::InputPriority_UI);

/*

bool CUIServer::OnKeyDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	return CEGUISystem->getDefaultGUIContext().injectKeyDown((CEGUI::Key::Scan)((const Event::KeyDown&)Event).ScanCode);
}
//---------------------------------------------------------------------

bool CUIServer::OnKeyUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	return CEGUISystem->getDefaultGUIContext().injectKeyUp((CEGUI::Key::Scan)((const Event::KeyDown&)Event).ScanCode);
}
//---------------------------------------------------------------------

bool CUIServer::OnCharInput(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	return CEGUISystem->getDefaultGUIContext().injectChar(((const Event::CharInput&)Event).Char);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseMove(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const Event::MouseMove& Ev = (const Event::MouseMove&)Event;
	return CEGUISystem->getDefaultGUIContext().injectMousePosition((float)Ev.X, (float)Ev.Y);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseBtnDown(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	CEGUI::MouseButton Btn = (CEGUI::MouseButton)(((const Event::MouseBtnDown&)Event).Button);
	return CEGUISystem->getDefaultGUIContext().injectMouseButtonDown(Btn);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseBtnUp(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	CEGUI::MouseButton Btn = (CEGUI::MouseButton)(((const Event::MouseBtnUp&)Event).Button);
	return CEGUISystem->getDefaultGUIContext().injectMouseButtonUp(Btn);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseWheel(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//???!!!mul by some coeff!?
	return CEGUISystem->getDefaultGUIContext().injectMouseWheelChange((float)((const Event::MouseWheel&)Event).Delta);
}
//---------------------------------------------------------------------
*/

}
