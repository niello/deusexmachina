#include "UIServer.h"

#include "Window.h"
#include <Events/EventManager.h>
#include <Input/InputServer.h>
#include <Input/Events/KeyDown.h>
#include <Input/Events/KeyUp.h>
#include <Input/Events/CharInput.h>
#include <Input/Events/MouseMove.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseWheel.h>
//#include <gfx2/ngfxserver2.h> //!!!when N2 renderer!
#include <gfx2/nd3d9server.h>

#include <UI/CEGUI/CEGUINebula2Logger.h>
#include <UI/CEGUI/CEGUINebula2ResourceProvider.h>

#include <CEGUISystem.h>
#include <CEGUIImageset.h>
#include <CEGUIFont.h>
#include <CEGUIScheme.h>
#include <falagard/CEGUIFalWidgetLookManager.h>
#include <CEGUIWindowManager.h>
#include <CEGUIFontManager.h>
//#include <CEGUIScriptModule.h> //???need?
#include <RendererModules/Direct3D9/CEGUIDirect3D9Renderer.h> //!!!use N2 renderer!
#include <XMLParserModules/TinyXML2Parser/CEGUITinyXML2Parser.h>

namespace UI
{
ImplementRTTI(UI::CUIServer, Core::CRefCounted);
__ImplementSingleton(UI::CUIServer);

CUIServer::CUIServer()
{
	n_assert(!Singleton);
	Singleton = this;

	n_assert2(nD3D9Server::Instance(), "There is no initialized nD3D9Server, can't use default renderer");

	Logger = n_new(CEGUI::CNebula2Logger);
	//!!!N2 renderer!
	Renderer = &CEGUI::Direct3D9Renderer::create(nD3D9Server::Instance()->GetD3DDevice());
	ResourceProvider = n_new(CEGUI::CNebula2ResourceProvider);
	Parser = n_new(CEGUI::TinyXML2Parser); //???delete?

	CEGUI::System::create(*Renderer, ResourceProvider, Parser);
	CEGUISystem = &CEGUI::System::getSingleton();

	CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Warnings);

	//!!!to config!
	ResourceProvider->setResourceGroupDirectory("schemes", "cegui:schemes/");
	ResourceProvider->setResourceGroupDirectory("imagesets", "cegui:imagesets/");
	ResourceProvider->setResourceGroupDirectory("fonts", "cegui:fonts/");
	ResourceProvider->setResourceGroupDirectory("layouts", "cegui:layouts/");
	ResourceProvider->setResourceGroupDirectory("looknfeels", "cegui:looknfeel/");
	ResourceProvider->setResourceGroupDirectory("lua_scripts", "cegui:lua_scripts/");
	CEGUI::Imageset::setDefaultResourceGroup("imagesets");
	CEGUI::Font::setDefaultResourceGroup("fonts");
	CEGUI::Scheme::setDefaultResourceGroup("schemes");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
	CEGUI::WindowManager::setDefaultResourceGroup("layouts");
	//CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
	nGfxServer2::Instance()->SetCursorVisibility(nGfxServer2::Custom);

	// For correct bool return from injects app should have a fullscreen DefaultWindow
	// as layout root with the MousePassThroughEnabled property set to true.
	SUBSCRIBE_INPUT_EVENT(KeyDown, CUIServer, OnKeyDown, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(KeyUp, CUIServer, OnKeyUp, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(CharInput, CUIServer, OnCharInput, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseMove, CUIServer, OnMouseMove, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseBtnDown, CUIServer, OnMouseBtnDown, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseBtnUp, CUIServer, OnMouseBtnUp, Input::InputPriority_UI);
	SUBSCRIBE_INPUT_EVENT(MouseWheel, CUIServer, OnMouseWheel, Input::InputPriority_UI);
}
//---------------------------------------------------------------------

CUIServer::~CUIServer()
{
	Screens.Clear();

	CEGUI::System::destroy();
	n_delete(Parser);
	n_delete(ResourceProvider);
	//!!!destroy resource provider etc (mb image codec)!
	CEGUI::Direct3D9Renderer::destroy(*Renderer);
	n_delete(Logger);

	n_assert(Singleton);
	Singleton = NULL;
}
//---------------------------------------------------------------------

void CUIServer::Trigger()
{
	nArray<CEGUI::Event::Connection>::iterator It = ConnectionsToDisconnect.Begin();
	for (; It != ConnectionsToDisconnect.End(); It++)
		(*It)->disconnect();
	ConnectionsToDisconnect.Clear();

	CEGUISystem->injectTimePulse((float)FrameTime);

	EventMgr->FireEvent(CStrID("OnUIUpdate"));
}
//---------------------------------------------------------------------

void CUIServer::Render()
{
	//???need? check what if don't clear buffer!
	nGfxServer2::Instance()->Clear(nGfxServer2::DepthBuffer, 0, 1.f, 0);
	CEGUI::System::getSingleton().renderGUI();
}
//---------------------------------------------------------------------

bool CUIServer::OnKeyDown(const Events::CEventBase& Event)
{
	return CEGUISystem->injectKeyDown(((const Event::KeyDown&)Event).ScanCode);
}
//---------------------------------------------------------------------

bool CUIServer::OnKeyUp(const Events::CEventBase& Event)
{
	return CEGUISystem->injectKeyUp(((const Event::KeyDown&)Event).ScanCode);
}
//---------------------------------------------------------------------

bool CUIServer::OnCharInput(const Events::CEventBase& Event)
{
	return CEGUISystem->injectChar(((const Event::CharInput&)Event).Char);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseMove(const Events::CEventBase& Event)
{
	//CEGUISystem->injectMouseLeaves
	const Event::MouseMove& Ev = (const Event::MouseMove&)Event;
	return CEGUISystem->injectMousePosition((float)Ev.X, (float)Ev.Y);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseBtnDown(const Events::CEventBase& Event)
{
	CEGUI::MouseButton Btn = (CEGUI::MouseButton)(((const Event::MouseBtnDown&)Event).Button);
	return CEGUISystem->injectMouseButtonDown(Btn);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseBtnUp(const Events::CEventBase& Event)
{
	CEGUI::MouseButton Btn = (CEGUI::MouseButton)(((const Event::MouseBtnUp&)Event).Button);
	return CEGUISystem->injectMouseButtonUp(Btn);
}
//---------------------------------------------------------------------

bool CUIServer::OnMouseWheel(const Events::CEventBase& Event)
{
	//???!!!mul by some coeff!?
	return CEGUISystem->injectMouseWheelChange((float)((const Event::MouseWheel&)Event).Delta);
}
//---------------------------------------------------------------------

bool CUIServer::RegisterScreen(CStrID Name, CWindow* pScreen)
{
	if (Screens.Contains(Name)) FAIL;
	Screens.Add(Name, pScreen);
	OK;
}
//---------------------------------------------------------------------

void CUIServer::LoadScheme(const nString& ResourceFile)
{
	CEGUI::SchemeManager::getSingleton().create(ResourceFile.Get());
}
//---------------------------------------------------------------------

//CEGUI::Font& 
void CUIServer::LoadFont(const nString& ResourceFile)
{
	CEGUI::FontManager::getSingleton().create(ResourceFile.Get());
}
//---------------------------------------------------------------------

void CUIServer::DestroyWindow(CWindow* pWindow)
{
	CEGUI::WindowManager::getSingleton().destroyWindow(pWindow->GetWnd());
}
//---------------------------------------------------------------------

void CUIServer::SetRootScreen(CWindow* pWindow)
{
	n_assert(pWindow);
	CurrRootScreen = pWindow;
	CEGUISystem->setGUISheet(pWindow->GetWnd());
}
//---------------------------------------------------------------------

void CUIServer::SetRootWindow(CEGUI::Window* pWindow)
{
	n_assert(pWindow != NULL);
	CEGUISystem->setGUISheet(pWindow);
}
//---------------------------------------------------------------------

CEGUI::Window* CUIServer::GetRootWindow() const
{
	return CEGUISystem->getGUISheet();
}
//---------------------------------------------------------------------

void CUIServer::ShowGUI()
{
	if (CEGUISystem->getGUISheet() != NULL)
		CEGUISystem->getGUISheet()->setVisible(true);
}
//---------------------------------------------------------------------

void CUIServer::HideGUI()
{
	if (CEGUISystem->getGUISheet() != NULL)
		CEGUISystem->getGUISheet()->setVisible(false);
}
//---------------------------------------------------------------------

void CUIServer::SetDefaultMouseCursor(const nString& schemeName, const nString& cursorName)
{
	CEGUISystem->setDefaultMouseCursor(schemeName.Get(), cursorName.Get());
}
//---------------------------------------------------------------------

CEGUI::Point CUIServer::GetMousePosition() const
{
	return CEGUI::MouseCursor::getSingleton().getPosition();
}
//---------------------------------------------------------------------

CEGUI::UVector2 CUIServer::GetMousePositionU() const
{
	CEGUI::Point Pos = CEGUI::MouseCursor::getSingleton().getPosition();
	return CEGUI::UVector2(CEGUI::UDim(0.f, Pos.d_x), CEGUI::UDim(0.f, Pos.d_y));
}
//---------------------------------------------------------------------

bool CUIServer::IsMouseOverGUI() const
{
	return CEGUISystem->getWindowContainingMouse() != CEGUISystem->getGUISheet();
}
//---------------------------------------------------------------------

} //namespace AI