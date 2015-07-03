#include "UIServer.h"

#include <UI/UIWindow.h>
#include <Events/EventServer.h>
#include <Input/InputServer.h>
#include <Input/Events/KeyDown.h>
#include <Input/Events/KeyUp.h>
#include <Input/Events/CharInput.h>
#include <Input/Events/MouseMove.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseWheel.h>

#include <UI/CEGUI/DEMLogger.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMResourceProvider.h>

// CEGUI uses insecure function in a template class -_-
#pragma warning(push)
#pragma warning(disable : 4996)       // _CRT_INSECURE_DEPRECATE, VS8: old string routines are deprecated

#include <CEGUI/System.h>
#include <CEGUI/Font.h>
#include <CEGUI/Scheme.h>
#include <CEGUI/falagard/WidgetLookManager.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/FontManager.h>
#include <CEGUI/XMLParserModules/TinyXML2/XMLParser.h>

namespace UI
{
__ImplementClassNoFactory(UI::CUIServer, Core::CObject);
__ImplementSingleton(UI::CUIServer);

CUIServer::CUIServer()
{
	n_assert(!Singleton);
	Singleton = this;

	Logger = n_new(CEGUI::CDEMLogger);
	//Renderer = &CEGUI::CDEMRenderer::create(GPUDriver, SwapChainID);
	ResourceProvider = n_new(CEGUI::CDEMResourceProvider);
	XMLParser = n_new(CEGUI::TinyXML2Parser);

	//CEGUI::System::create(*Renderer, ResourceProvider, XMLParser);
	CEGUISystem = &CEGUI::System::getSingleton();

	CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Warnings);

	//!!!to config!
	ResourceProvider->setResourceGroupDirectory("schemes", "CEGUI:schemes/");
	ResourceProvider->setResourceGroupDirectory("imagesets", "CEGUI:imagesets/");
	ResourceProvider->setResourceGroupDirectory("fonts", "CEGUI:fonts/");
	ResourceProvider->setResourceGroupDirectory("layouts", "CEGUI:layouts/");
	ResourceProvider->setResourceGroupDirectory("looknfeels", "CEGUI:looknfeel/");
	ResourceProvider->setResourceGroupDirectory("lua_scripts", "CEGUI:lua_scripts/");
	CEGUI::Font::setDefaultResourceGroup("fonts");
	CEGUI::Scheme::setDefaultResourceGroup("schemes");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
	CEGUI::WindowManager::setDefaultResourceGroup("layouts");

	// For correct bool return from injects app should have a fullscreen DefaultWindow
	// as layout root with the MousePassThroughEnabled property set to true.
	SUBSCRIBE_INPUT_EVENT(KeyDown, CUIServer, OnKeyDown, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(KeyUp, CUIServer, OnKeyUp, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(CharInput, CUIServer, OnCharInput, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseMove, CUIServer, OnMouseMove, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CUIServer, OnMouseBtnDown, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CUIServer, OnMouseBtnUp, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseWheel, CUIServer, OnMouseWheel, Input::InputPriority_UI);
	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CUIServer, OnDeviceLost);
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CUIServer, OnDeviceReset);
}
//---------------------------------------------------------------------

CUIServer::~CUIServer()
{
	n_assert(Singleton);

	Screens.Clear();

	CEGUI::System::destroy();
	n_delete(XMLParser);
	n_delete(ResourceProvider);
	//!!!destroy resource provider etc (mb image codec)!
//	CEGUI::Direct3D9Renderer::destroy(*Renderer);
	n_delete(Logger);

	Singleton = NULL;
}
//---------------------------------------------------------------------

void CUIServer::Trigger(float FrameTime)
{
	CArray<CEGUI::Event::Connection>::CIterator It = ConnectionsToDisconnect.Begin();
	for (; It != ConnectionsToDisconnect.End(); It++)
		(*It)->disconnect();
	ConnectionsToDisconnect.Clear();

	CEGUISystem->injectTimePulse(FrameTime);
	CEGUISystem->getDefaultGUIContext().injectTimePulse(FrameTime);

	EventSrv->FireEvent(CStrID("OnUIUpdate"));
}
//---------------------------------------------------------------------

//???need this method? see UIRenderer
void CUIServer::Render()
{
	CEGUI::System::getSingleton().renderAllGUIContexts();
}
//---------------------------------------------------------------------

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

bool CUIServer::OnDeviceLost(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//Renderer->preD3DReset();
	OK;
}
//---------------------------------------------------------------------

bool CUIServer::OnDeviceReset(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	//Renderer->postD3DReset();
	OK;
}
//---------------------------------------------------------------------

bool CUIServer::RegisterScreen(CStrID Name, CUIWindow* pScreen)
{
	if (Screens.Contains(Name)) FAIL;
	Screens.Add(Name, pScreen);
	OK;
}
//---------------------------------------------------------------------

void CUIServer::LoadScheme(const CString& ResourceFile)
{
	CEGUI::SchemeManager::getSingleton().createFromFile(ResourceFile.CStr());
}
//---------------------------------------------------------------------

//CEGUI::Font& 
void CUIServer::LoadFont(const CString& ResourceFile)
{
	CEGUI::FontManager::getSingleton().createFromFile(ResourceFile.CStr());
}
//---------------------------------------------------------------------

void CUIServer::DestroyWindow(CUIWindow* pWindow)
{
	CEGUI::WindowManager::getSingleton().destroyWindow(pWindow->GetWnd());
}
//---------------------------------------------------------------------

void CUIServer::SetRootScreen(CUIWindow* pWindow)
{
	n_assert(pWindow);
	CurrRootScreen = pWindow;
	CEGUISystem->getDefaultGUIContext().setRootWindow(pWindow->GetWnd());
	CEGUISystem->getDefaultGUIContext().updateWindowContainingMouse();
}
//---------------------------------------------------------------------

void CUIServer::SetRootWindow(CEGUI::Window* pWindow)
{
	n_assert(pWindow != NULL);
	CEGUISystem->getDefaultGUIContext().setRootWindow(pWindow);
}
//---------------------------------------------------------------------

CEGUI::Window* CUIServer::GetRootWindow() const
{
	return CEGUISystem->getDefaultGUIContext().getRootWindow();
}
//---------------------------------------------------------------------

void CUIServer::ShowGUI()
{
	CEGUI::Window* pRoot = CEGUISystem->getDefaultGUIContext().getRootWindow();
	if (pRoot) pRoot->setVisible(true);
}
//---------------------------------------------------------------------

void CUIServer::HideGUI()
{
	CEGUI::Window* pRoot = CEGUISystem->getDefaultGUIContext().getRootWindow();
	if (pRoot) pRoot->setVisible(false);
}
//---------------------------------------------------------------------

void CUIServer::SetDefaultMouseCursor(const CString& ImageName)
{
	CEGUISystem->getDefaultGUIContext().getMouseCursor().setDefaultImage(ImageName.CStr());
}
//---------------------------------------------------------------------

void CUIServer::ShowMouseCursor()
{
	CEGUISystem->getDefaultGUIContext().getMouseCursor().show();
}
//---------------------------------------------------------------------

void CUIServer::HideMouseCursor()
{
	CEGUISystem->getDefaultGUIContext().getMouseCursor().hide();
}
//---------------------------------------------------------------------

CEGUI::Vector2f CUIServer::GetMousePosition() const
{
	return CEGUISystem->getDefaultGUIContext().getMouseCursor().getPosition();
}
//---------------------------------------------------------------------

CEGUI::UVector2 CUIServer::GetMousePositionU() const
{
	CEGUI::Vector2f Pos = CEGUISystem->getDefaultGUIContext().getMouseCursor().getPosition();
	return CEGUI::UVector2(CEGUI::UDim(0.f, Pos.d_x), CEGUI::UDim(0.f, Pos.d_y));
}
//---------------------------------------------------------------------

bool CUIServer::IsMouseOverGUI() const
{
	return CEGUISystem->getDefaultGUIContext().getWindowContainingMouse() !=
		CEGUISystem->getDefaultGUIContext().getRootWindow();
}
//---------------------------------------------------------------------

}

#pragma warning(pop)
