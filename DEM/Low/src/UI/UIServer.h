#pragma once
#include <Events/EventsFwd.h>
#include <Data/Ptr.h>
#include <Core/RTTI.h>
#include <map>

// UI server (CEGUI launcher & manager). This server creates top-level screens and manages
// their switching (or provides switching functionality to application states).

//???May be should control global state of UI (all tips above IAOs are visible or not etc). Should load windows.

namespace CEGUI
{
	class CDEMLogger;
	class CDEMRenderer;
	class CDEMResourceProvider;
	class XMLParser;
	class System;
}

namespace Data
{
	class CParams;
}

namespace Render
{
	class CGPUDriver;
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

	CEGUI::CDEMRenderer*         Renderer;
	CEGUI::System*               CEGUISystem;
	CEGUI::CDEMLogger*           Logger;
	CEGUI::CDEMResourceProvider* ResourceProvider;
	CEGUI::XMLParser*            XMLParser;

	std::map<const DEM::Core::CRTTI*, std::vector<PUIWindow>> WindowPool; // Preloaded windows suitable for reuse // TODO: std::type_index?

	DECLARE_EVENT_HANDLER(OnRenderDeviceLost, OnDeviceLost);
	DECLARE_EVENT_HANDLER(OnRenderDeviceReset, OnDeviceReset);

	PUIWindow FindReusableWindow(const DEM::Core::CRTTI* Type);

public:

	CUIServer(Render::CGPUDriver& GPU, const Data::CParams* pSettings = nullptr);
	~CUIServer();
	
	// Internal use, set by config
	void       LoadScheme(const char* pResourceFile);
	void       LoadFont(const char* pResourceFile);
	//!!!create dynamic fonts! see article!

	void       Trigger(float FrameTime);

	PUIContext CreateContext(float Width, float Height, DEM::Sys::COSWindow* pHostWindow = nullptr);

	template<typename T>
	Ptr<T> CreateWindow()
	{
		static_assert(std::is_base_of_v<CUIWindow, T>, "CUIServer::CreateWindow() can create only CUIWindow and subclasses");
		auto Result = FindReusableWindow(&T::RTTI);
		return Ptr<T>(Result ? static_cast<T*>(Result.Get()) : n_new(T));
	}

	void ReleaseWindow(PUIWindow Wnd);
	void DestroyAllReusableWindows();

	template<typename T>
	void DestroyReusableWindows()
	{
		WindowPool.erase(&T::RTTI);
	}
};

}
