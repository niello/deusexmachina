#include "UIServer.h"

#include <UI/UIContext.h>
#include <Events/EventServer.h>
#include <Events/Subscription.h>
#include <Data/DataArray.h>

#include <Render/GPUDriver.h>

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
__ImplementSingleton(UI::CUIServer);

CUIServer::CUIServer(const CUISettings& Settings)
{
	__ConstructSingleton;

	Logger = n_new(CEGUI::CDEMLogger);
	Logger->setLoggingLevel(CEGUI::LoggingLevel::Warning); //???to settings?

	Renderer = &CEGUI::CDEMRenderer::create(*Settings.GPU, *Settings.Effect);

	//!!!TMP!
	// TODO: CEGUI implement - get display size from GPU output 0 or don't use display size at all
	// FIXME: set correct values (whatever it is intended to be)
	//???here or inside a renderer constructor?
	Renderer->setDisplaySize(CEGUI::Sizef(1024.f, 768.f));

	ResourceProvider = n_new(CEGUI::CDEMResourceProvider);
	XMLParser = n_new(CEGUI::TinyXML2Parser);

	CEGUI::System::create(*Renderer, ResourceProvider, XMLParser);
	CEGUISystem = CEGUI::System::getSingletonPtr();

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

	Data::PDataArray ResourcesToLoad;

	// TODO: move to CEGUI scheme, it already has capability of font autoloading
	if (Settings.LoadOnStartup->Get<Data::PDataArray>(ResourcesToLoad, CStrID("Fonts")))
		for (UPTR i = 0; i < ResourcesToLoad->GetCount(); ++i)
			LoadFont(ResourcesToLoad->Get<CString>(i).CStr());

	if (Settings.LoadOnStartup->Get<Data::PDataArray>(ResourcesToLoad, CStrID("Schemes")))
		for (UPTR i = 0; i < ResourcesToLoad->GetCount(); ++i)
			LoadScheme(ResourcesToLoad->Get<CString>(i).CStr());

	if (Settings.DefaultCursor.IsValid())
		CEGUISystem->setDefaultCursorName(Settings.DefaultCursor.CStr());

	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CUIServer, OnDeviceLost);
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CUIServer, OnDeviceReset);
}
//---------------------------------------------------------------------

CUIServer::~CUIServer()
{
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
	for (auto pCtx : CEGUISystem->getGUIContexts())
		pCtx->injectTimePulse(FrameTime);

	EventSrv->FireEvent(CStrID("OnUIUpdate"));
}
//---------------------------------------------------------------------

PUIContext CUIServer::CreateContext(const CUIContextSettings& Settings)
{
	//???to UI context constructor? CEGUI context destroyed there, so must be created there too!
	//???W+H+Window instead of CUIContextSettings?
	CEGUI::RenderTarget* pTarget = Renderer->createViewportTarget(Settings.Width, Settings.Height);
	return n_new(CUIContext(&CEGUISystem->createGUIContext(*pTarget), Settings.HostWindow.Get()));
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
