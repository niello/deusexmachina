#pragma once
#include <Core/Object.h>
#include <Data/Singleton.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <Data/Dictionary.h>
#include <CEGUI/Event.h>

// UI server (CEGUI launcher & manager). This server creates top-level screens and manages
// their switching (or provides switching functionality to application states).

//???May be should control global state of UI (all tips above IAOs are visible or not etc). Should load windows.

namespace CEGUI
{
	class CDEMLogger;
	class CDEMRenderer;
	class CDEMResourceProvider;
	class TinyXML2Parser;
}

namespace Data
{
	typedef Ptr<class CParams> PParams;
}

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
	typedef Ptr<class CShader> PShader;
}

namespace DEM { namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}}

namespace UI
{
typedef Ptr<class CUIWindow> PUIWindow;
typedef Ptr<class CUIContext> PUIContext;

#define UISrv UI::CUIServer::Instance()

// Service-wide settings
struct CUISettings
{
	Render::PGPUDriver	GPU;
	CStrID				VertexShaderID;
	CStrID				PixelShaderRegularID;
	CStrID				PixelShaderOpaqueID;
	Data::PParams		ResourceGroups;
};

struct CUIContextSettings
{
	DEM::Sys::POSWindow	HostWindow;
	float				Width;
	float				Height;
};

class CUIServer: public Core::CObject
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CUIServer);

private:

	CEGUI::CDEMRenderer*				Renderer;
	CEGUI::System*						CEGUISystem;
	CEGUI::CDEMLogger*					Logger;
	CEGUI::CDEMResourceProvider*		ResourceProvider;
	CEGUI::TinyXML2Parser*				XMLParser;

	CArray<CEGUI::Event::Connection>	ConnectionsToDisconnect;

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CUIServer(const CUISettings& Settings, const CUIContextSettings& DefaultContextSettings);
	~CUIServer();
	
	// Internal use, set by config
	void			LoadScheme(const char* pResourceFile);
	void			LoadFont(const char* pResourceFile);
	//!!!create dynamic fonts! see article!

	void			Trigger(float FrameTime);

	PUIContext		CreateContext(const CUIContextSettings& Settings); //!!!params!
	void			DestroyContext(PUIContext Context); //!!!params!
	
	// Event will be disconnected at the beginning of the next GUI update loop.
	// Attention! This method is not thread safe. You must call it only from GUI thread.
	void			DelayedDisconnect(CEGUI::Event::Connection Connection) { ConnectionsToDisconnect.Add(Connection); }
};

}
