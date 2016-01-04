#include "UIServer.h"

#include <UI/UIContext.h>
#include <Events/EventServer.h>

#include <UI/CEGUI/DEMLogger.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMResourceProvider.h>
#include <UI/CEGUI/DEMViewportTarget.h>

// CEGUI uses insecure function in a template class -_-
#pragma warning(push)
#pragma warning(disable : 4996) // _CRT_INSECURE_DEPRECATE, VS8: old string routines are deprecated

#include <CEGUI/System.h>
#include <CEGUI/Font.h>
#include <CEGUI/Scheme.h>
#include <CEGUI/falagard/WidgetLookManager.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/FontManager.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/XMLParserModules/TinyXML2/XMLParser.h>

namespace UI
{
__ImplementClassNoFactory(UI::CUIServer, Core::CObject);
__ImplementSingleton(UI::CUIServer);

CUIServer::CUIServer(Render::CGPUDriver& GPUDriver, int SwapChainID, const char* pVertexShaderURI, const char* pPixelShaderURI)
{
	__ConstructSingleton;

	Logger = n_new(CEGUI::CDEMLogger);
	Renderer = &CEGUI::CDEMRenderer::create(GPUDriver, SwapChainID, pVertexShaderURI, pPixelShaderURI);
	ResourceProvider = n_new(CEGUI::CDEMResourceProvider);
	XMLParser = n_new(CEGUI::TinyXML2Parser);

	CEGUI::System::create(*Renderer, ResourceProvider, XMLParser);
	CEGUISystem = CEGUI::System::getSingletonPtr();

	CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Warnings);

	DefaultContext = n_new(CUIContext);
	DefaultContext->Init(&CEGUISystem->getDefaultGUIContext());

	//!!!to config!
	ResourceProvider->setResourceGroupDirectory("schemes", "CEGUI:schemes/");
	ResourceProvider->setResourceGroupDirectory("imagesets", "CEGUI:imagesets/");
	ResourceProvider->setResourceGroupDirectory("fonts", "CEGUI:fonts/");
	ResourceProvider->setResourceGroupDirectory("layouts", "CEGUI:layouts/");
	ResourceProvider->setResourceGroupDirectory("looknfeels", "CEGUI:looknfeel/");
	ResourceProvider->setResourceGroupDirectory("lua_scripts", "CEGUI:lua_scripts/");
	CEGUI::Font::setDefaultResourceGroup("fonts");
	CEGUI::Scheme::setDefaultResourceGroup("schemes");
	CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
	CEGUI::WindowManager::setDefaultResourceGroup("layouts");

	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CUIServer, OnDeviceLost);
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CUIServer, OnDeviceReset);
}
//---------------------------------------------------------------------

CUIServer::~CUIServer()
{
	DefaultContext = NULL;

	CEGUI::System::destroy();
	n_delete(XMLParser);
	n_delete(ResourceProvider);
	CEGUI::CDEMRenderer::destroy(*Renderer);
	n_delete(Logger);

	__DestructSingleton;
}
//---------------------------------------------------------------------

void CUIServer::Trigger(float FrameTime)
{
	CArray<CEGUI::Event::Connection>::CIterator It = ConnectionsToDisconnect.Begin();
	for (; It != ConnectionsToDisconnect.End(); It++)
		(*It)->disconnect();
	ConnectionsToDisconnect.Clear();

	CEGUI::WindowManager::getSingleton().cleanDeadPool();

	CEGUISystem->injectTimePulse(FrameTime);
	CEGUISystem->getDefaultGUIContext().injectTimePulse(FrameTime); //???subscribe all contexts on some time event or store collection here?
	//!!!inject in all contexts!

	EventSrv->FireEvent(CStrID("OnUIUpdate"));
}
//---------------------------------------------------------------------

void CUIServer::LoadScheme(const char* pResourceFile)
{
	CEGUI::SchemeManager::getSingleton().createFromFile(pResourceFile);
}
//---------------------------------------------------------------------

void CUIServer::LoadFont(const char* pResourceFile)
{
	CEGUI::FontManager::getSingleton().createFromFile(pResourceFile);
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

}

#pragma warning(pop)
