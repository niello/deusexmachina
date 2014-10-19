#include "UIServer.h"

#include "Window.h"
#include <Render/RenderServer.h>
#include <Events/EventServer.h>
#include <Input/InputServer.h>
#include <Input/Events/KeyDown.h>
#include <Input/Events/KeyUp.h>
// CEGUI uses insecure function in a template class -_-
#pragma warning(push)
#pragma warning(disable : 4996)       // _CRT_INSECURE_DEPRECATE, VS8: old string routines are deprecated

#include <Input/Events/CharInput.h>
#include <Input/Events/MouseMove.h>
#include <Input/Events/MouseBtnDown.h>
#include <Input/Events/MouseBtnUp.h>
#include <Input/Events/MouseWheel.h>

#include <UI/CEGUI/CEGUINebula2Logger.h>
#include <UI/CEGUI/CEGUINebula2ResourceProvider.h>

#include <CEGUISystem.h>
#include <CEGUIImageset.h>
#include <CEGUIFont.h>
#include <CEGUIScheme.h>
#include <falagard/CEGUIFalWidgetLookManager.h>
#include <CEGUIWindowManager.h>
#include <CEGUIFontManager.h>
#include <RendererModules/Direct3D9/CEGUIDirect3D9Renderer.h> //!!!use N2 renderer!
#include <XMLParserModules/TinyXML2Parser/CEGUITinyXML2Parser.h>

namespace UI
{
__ImplementClassNoFactory(UI::CUIServer, Core::CObject);
__ImplementSingleton(UI::CUIServer);

CUIServer::CUIServer()
{
	n_assert(!Singleton);
	Singleton = this;

	Logger = n_new(CEGUI::CNebula2Logger);
	//!!!N2 renderer!
	Renderer = &CEGUI::Direct3D9Renderer::create(RenderSrv->GetD3DDevice());
	ResourceProvider = n_new(CEGUI::CNebula2ResourceProvider);
	Parser = n_new(CEGUI::TinyXML2Parser); //???delete?

	CEGUI::System::create(*Renderer, ResourceProvider, Parser);
	CEGUISystem = &CEGUI::System::getSingleton();

	CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Warnings);

	//!!!to config!
	ResourceProvider->setResourceGroupDirectory("schemes", "CEGUI:schemes/");
	ResourceProvider->setResourceGroupDirectory("imagesets", "CEGUI:imagesets/");
	ResourceProvider->setResourceGroupDirectory("fonts", "CEGUI:fonts/");
	ResourceProvider->setResourceGroupDirectory("layouts", "CEGUI:layouts/");
	ResourceProvider->setResourceGroupDirectory("looknfeels", "CEGUI:looknfeel/");
	ResourceProvider->setResourceGroupDirectory("lua_scripts", "CEGUI:lua_scripts/");
	CEGUI::Imageset::setDefaultResourceGroup("imagesets");
	CEGUI::Font::setDefaultResourceGroup("fonts");
	CEGUI::Scheme::setDefaultResourceGroup("schemes");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
	CEGUI::WindowManager::setDefaultResourceGroup("layouts");
	//CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");

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
	n_delete(Parser);
	n_delete(ResourceProvider);
	//!!!destroy resource provider etc (mb image codec)!
	CEGUI::Direct3D9Renderer::destroy(*Renderer);
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

	EventSrv->FireEvent(CStrID("OnUIUpdate"));
}
//---------------------------------------------------------------------

//???need this method? see UIRenderer
void CUIServer::Render()
{
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

bool CUIServer::OnDeviceLost(const Events::CEventBase& Ev)
{
	Renderer->preD3DReset();
	OK;
}
//---------------------------------------------------------------------

bool CUIServer::OnDeviceReset(const Events::CEventBase& Ev)
{
	Renderer->postD3DReset();
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
	CEGUI::SchemeManager::getSingleton().create(ResourceFile.CStr());
}
//---------------------------------------------------------------------

//CEGUI::Font& 
void CUIServer::LoadFont(const CString& ResourceFile)
{
	CEGUI::FontManager::getSingleton().create(ResourceFile.CStr());
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
	CEGUISystem->setGUISheet(pWindow->GetWnd());
	CEGUISystem->updateWindowContainingMouse();
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

void CUIServer::SetDefaultMouseCursor(const CString& schemeName, const CString& cursorName)
{
	CEGUISystem->setDefaultMouseCursor(schemeName.CStr(), cursorName.CStr());
}
//---------------------------------------------------------------------

void CUIServer::ShowMouseCursor()
{
	CEGUI::MouseCursor::getSingleton().show();
}
//---------------------------------------------------------------------

void CUIServer::HideMouseCursor()
{
	CEGUI::MouseCursor::getSingleton().hide();
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

}

#pragma warning(pop)
