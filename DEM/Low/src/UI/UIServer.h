#pragma once
#include <Events/EventsFwd.h>
#include <Data/Ptr.h>
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
	class CParams;
}

namespace Render
{
	class CGPUDriver;
	class CEffect;
}

namespace DEM::Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}

namespace UI
{
typedef Ptr<class CUIWindow> PUIWindow;
typedef Ptr<class CUIContext> PUIContext;

class CUIServer final
{
private:

	CEGUI::CDEMRenderer*				Renderer;
	CEGUI::System*						CEGUISystem;
	CEGUI::CDEMLogger*					Logger;
	CEGUI::CDEMResourceProvider*		ResourceProvider;
	CEGUI::TinyXML2Parser*				XMLParser;

	std::vector<CEGUI::Event::Connection> ConnectionsToDisconnect;

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CUIServer(Render::CGPUDriver& GPU, Render::CEffect& Effect, const Data::CParams* pSettings = nullptr);
	~CUIServer();
	
	// Internal use, set by config
	void       LoadScheme(const char* pResourceFile);
	void       LoadFont(const char* pResourceFile);
	//!!!create dynamic fonts! see article!

	void       Trigger(float FrameTime);

	PUIContext CreateContext(float Width, float Height, DEM::Sys::COSWindow* pHostWindow = nullptr);
	
	// Event will be disconnected at the beginning of the next GUI update loop.
	// Attention! This method is not thread safe. You must call it only from GUI thread.
	void       DelayedDisconnect(CEGUI::Event::Connection Connection) { ConnectionsToDisconnect.push_back(Connection); }
};

}
