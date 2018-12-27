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

CUIServer::CUIServer(const CUISettings& Settings)
{
	__ConstructSingleton;

	Logger = n_new(CEGUI::CDEMLogger);
	Logger->setLoggingLevel(CEGUI::LoggingLevel::Warnings); //???to settings?

	Renderer = &CEGUI::CDEMRenderer::create(
		*Settings.GPUDriver, Settings.SwapChainID,
		Settings.DefaultContextWidth, Settings.DefaultContextHeight,
		Settings.VertexShader, Settings.PixelShaderRegular, Settings.PixelShaderOpaque);
	ResourceProvider = n_new(CEGUI::CDEMResourceProvider);
	XMLParser = n_new(CEGUI::TinyXML2Parser);

	CEGUI::System::create(*Renderer, ResourceProvider, XMLParser);
	CEGUISystem = CEGUI::System::getSingletonPtr();

	DefaultContext = n_new(CUIContext);
	DefaultContext->Init(&CEGUISystem->getDefaultGUIContext());

	if (Settings.ResourceGroups.IsValidPtr())
	{
		for (UPTR i = 0; i < Settings.ResourceGroups->GetCount(); ++i)
		{
			Data::CParam& Prm = Settings.ResourceGroups->Get(i);
			ResourceProvider->setResourceGroupDirectory(Prm.GetName().CStr(), Prm.GetValue<CString>().CStr());
		}
	}

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
	for (; It != ConnectionsToDisconnect.End(); ++It)
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
