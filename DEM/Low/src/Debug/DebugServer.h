#pragma once
#include <Data/StringID.h>
#include <Data/Singleton.h>
#include <Events/EventsFwd.h>
#include <UI/UIContext.h>
#include <Data/Dictionary.h>

// Central point of all debug (and profiling/statistics???) functionality

namespace Debug
{
#define DbgSrv Debug::CDebugServer::Instance()

class CDebugServer: public DEM::Core::CObject
{
	RTTI_CLASS_DECL(Debug::CDebugServer, DEM::Core::CObject);
	__DeclareSingleton(CDebugServer);

private:

	struct CPlugin
	{
		std::string			UIResource;
		UI::PUIWindow		Window;
	};

	UI::PUIContext			UIContext;
	CDict<CStrID, CPlugin>	Plugins;

public:

	CDebugServer();
	~CDebugServer();

	bool RegisterPlugin(CStrID Name, const char* CppClassName, const char* UIResource);
	void SetUIContext(UI::PUIContext Context);
	void Trigger();
	void TogglePluginWindow(CStrID Name);
};

}
