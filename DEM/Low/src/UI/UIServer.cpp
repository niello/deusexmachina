#include "UIServer.h"

#include <UI/UIContext.h>
#include <Render/GPUDriver.h>
#include <Events/EventServer.h>
#include <Events/Subscription.h>
#include <Data/DataArray.h>

#include <UI/CEGUI/DEMLogger.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMResourceProvider.h>
#include <UI/CEGUI/DEMViewportTarget.h>

#include <CEGUI/System.h>
#include <CEGUI/text/Font.h>
#include <CEGUI/Scheme.h>
#include <CEGUI/SchemeManager.h>
#include <CEGUI/falagard/WidgetLookManager.h>
#include <CEGUI/WindowManager.h>
#include <CEGUI/FontManager.h>
#include <CEGUI/ImageManager.h>
#include <CEGUI/XMLParserModules/PugiXML/XMLParser.h>

namespace UI
{

CUIServer::CUIServer(Render::CGPUDriver& GPU, const Data::CParams* pSettings)
{
	if (CEGUI::System::getSingletonPtr())
	{
		::Sys::Error("CUIServer::CUIServer() > CEGUI is singleton-based, can't create second UI server!");
		return;
	}

	Logger = n_new(CEGUI::CDEMLogger);
	Logger->setLoggingLevel(CEGUI::LoggingLevel::Warning); //???to settings?

	Renderer = &CEGUI::CDEMRenderer::create(GPU);

	//!!!TMP!
	// TODO: CEGUI implement - get display size from GPU output 0 or don't use display size at all
	// FIXME: set correct values (whatever it is intended to be)
	//???here or inside a renderer constructor?
	Renderer->setDisplaySize(CEGUI::Sizef(1024.f, 768.f));

	ResourceProvider = n_new(CEGUI::CDEMResourceProvider);
	XMLParser = n_new(CEGUI::PugiXMLParser);

	CEGUI::System::create(*Renderer, ResourceProvider, XMLParser);
	CEGUISystem = CEGUI::System::getSingletonPtr();

	Data::PParams ResourceGroups;
	if (pSettings && pSettings->TryGet<Data::PParams>(ResourceGroups, CStrID("ResourceGroups")))
	{
		for (UPTR i = 0; i < ResourceGroups->GetCount(); ++i)
		{
			Data::CParam& Prm = ResourceGroups->Get(i);
			ResourceProvider->setResourceGroupDirectory(Prm.GetName().CStr(), Prm.GetValue<CString>().CStr());
		}
	}

	CEGUI::Font::setDefaultResourceGroup("fonts");
	CEGUI::Scheme::setDefaultResourceGroup("schemes");
	CEGUI::ImageManager::setImagesetDefaultResourceGroup("imagesets");
	CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
	CEGUI::WindowManager::setDefaultResourceGroup("layouts");

	Data::PParams LoadOnStartup;
	if (pSettings && pSettings->TryGet<Data::PParams>(LoadOnStartup, CStrID("LoadOnStartup")))
	{
		Data::PDataArray ResourcesToLoad;
		if (LoadOnStartup->TryGet<Data::PDataArray>(ResourcesToLoad, CStrID("Schemes")))
			for (UPTR i = 0; i < ResourcesToLoad->GetCount(); ++i)
				LoadScheme(ResourcesToLoad->Get<CString>(i).CStr());
	}

	const CString& DefaultCursor = pSettings ? pSettings->Get<CString>(CStrID("DefaultCursor"), CString::Empty) : CString::Empty;
	if (DefaultCursor.IsValid())
		CEGUISystem->setDefaultCursorName(DefaultCursor.CStr());

	const CString& DefaultTooltip = pSettings ? pSettings->Get<CString>(CStrID("DefaultTooltip"), CString::Empty) : CString::Empty;
	if (DefaultTooltip.IsValid())
		CEGUISystem->setDefaultTooltipType(DefaultTooltip.CStr());

	const CString& DefaultFont = pSettings ? pSettings->Get<CString>(CStrID("DefaultFont"), CString::Empty) : CString::Empty;
	if (DefaultFont.IsValid())
		CEGUISystem->setDefaultFontName(DefaultFont.CStr());

	SUBSCRIBE_PEVENT(OnRenderDeviceLost, CUIServer, OnDeviceLost);
	SUBSCRIBE_PEVENT(OnRenderDeviceReset, CUIServer, OnDeviceReset);
}
//---------------------------------------------------------------------

CUIServer::~CUIServer()
{
	WindowPool.clear();

	CEGUI::System::destroy();
	n_delete(XMLParser);
	n_delete(ResourceProvider);
	CEGUI::CDEMRenderer::destroy(*Renderer);
	n_delete(Logger);
}
//---------------------------------------------------------------------

void CUIServer::Trigger(float FrameTime)
{
	CEGUI::WindowManager::getSingleton().cleanDeadPool();

	CEGUISystem->injectTimePulse(FrameTime);

	EventSrv->FireEvent(CStrID("OnUIUpdate"));
}
//---------------------------------------------------------------------

PUIContext CUIServer::CreateContext(float Width, float Height, DEM::Sys::COSWindow* pHostWindow)
{
	return n_new(CUIContext(Width, Height, pHostWindow));
}
//---------------------------------------------------------------------

PUIWindow CUIServer::FindReusableWindow(const Core::CRTTI* Type)
{
	auto It = WindowPool.find(Type);
	if (It == WindowPool.cend() || It->second.empty()) return {};

	auto Result = std::move(It->second.back());
	It->second.pop_back();
	return std::move(Result);
}
//---------------------------------------------------------------------

void CUIServer::ReleaseWindow(PUIWindow Wnd)
{
	n_assert_dbg(Wnd && !Wnd->GetParent() && !Wnd->GetContext());
	if (!Wnd || Wnd->GetParent() || Wnd->GetContext()) return;
	WindowPool[Wnd->GetRTTI()].push_back(std::move(Wnd));
}
//---------------------------------------------------------------------

void CUIServer::DestroyAllReusableWindows()
{
	WindowPool.clear();
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
