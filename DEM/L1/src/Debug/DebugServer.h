#pragma once
#ifndef __DEM_L1_DEBUG_SERVER_H__
#define __DEM_L1_DEBUG_SERVER_H__

#include <Data/StringID.h>
#include <Data/Singleton.h>
#include <Events/EventsFwd.h>
#include <UI/UIWindow.h>
#include <Data/Dictionary.h>

// Central point of all debug (and profiling/statistics???) functionality

namespace Debug
{
#define DbgSrv Debug::CDebugServer::Instance()

class CDebugServer: public Core::CObject
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CDebugServer);

private:

	struct CPlugin
	{
		CString	UIResource;
		UI::PUIWindow		Window;
	};

	bool					UIAllowed;
	CDict<CStrID, CPlugin>	Plugins;

public:

	CDebugServer();
	~CDebugServer();

	bool RegisterPlugin(CStrID Name, const char* CppClassName, const char* UIResource);
	void AllowUI(bool Allow);
	void Trigger();
	void TogglePluginWindow(CStrID Name);
};

}

#endif
