#pragma once
#ifndef __DEM_L1_UI_SERVER_H__
#define __DEM_L1_UI_SERVER_H__

#include <Core/Object.h>
#include <Data/Singleton.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>
#include <Data/Dictionary.h>
#include <CEGUI/Event.h>
#include <CEGUI/Vector.h>

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

namespace UI
{
typedef Ptr<class CUIWindow> PUIWindow;
typedef Ptr<class CUIContext> PUIContext;

#define UISrv UI::CUIServer::Instance()

struct CUISettings
{
	Render::PGPUDriver	GPUDriver;
	Render::PShader		VertexShader;
	Render::PShader		PixelShaderRegular;
	Render::PShader		PixelShaderOpaque;

	//!!!default context concept is a big mistake! Hope there won't be the one in CEGUI 1.0
	int					SwapChainID;
	float				DefaultContextWidth;
	float				DefaultContextHeight;

	Data::PParams		ResourceGroups;
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

	PUIContext							DefaultContext; //???get rid of with CEGUI 1.0?

	CArray<CEGUI::Event::Connection>	ConnectionsToDisconnect;

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

public:

	CUIServer(const CUISettings& Settings);
	~CUIServer();
	
	// Internal use, set by config
	void			LoadScheme(const char* pResourceFile);
	void			LoadFont(const char* pResourceFile);
	//!!!create dynamic fonts! see article!

	void			Trigger(float FrameTime);

	PUIContext		GetDefaultContext() const; //???get rid of it with CEGUI 1.0?
	PUIContext		CreateContext(); //!!!params!
	void			DestroyContext(PUIContext Context); //!!!params!
	
	// Event will be disconnected at the beginning of the next GUI update loop.
	// Attention! This method is not thread safe. You must call it only from GUI thread.
	void			DelayedDisconnect(CEGUI::Event::Connection Connection) { ConnectionsToDisconnect.Add(Connection); }
};

}

#endif
