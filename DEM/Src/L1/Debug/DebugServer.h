#pragma once
#ifndef __DEM_L1_DEBUG_SERVER_H__
#define __DEM_L1_DEBUG_SERVER_H__

#include <Data/SimpleString.h>
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
		Data::CSimpleString	UIResource;
		UI::PUIWindow		Window;
	};

	bool					UIAllowed;
	CDict<CStrID, CPlugin>	Plugins;

	void TogglePluginWindow(CStrID Name);

	DECLARE_EVENT_HANDLER(ShowDebugConsole, OnShowDebugConsole);
	DECLARE_EVENT_HANDLER(ShowDebugWatcher, OnShowDebugWatcher);
	DECLARE_EVENT_HANDLER(DebugBreak, OnDebugBreak);

public:

	CDebugServer();
	~CDebugServer();

	bool RegisterPlugin(CStrID Name, LPCSTR CppClassName, LPCSTR UIResource);
	void AllowUI(bool Allow);
	void Trigger();
};

}

#endif
