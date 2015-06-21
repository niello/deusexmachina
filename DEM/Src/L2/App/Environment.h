#pragma once
#ifndef __DEM_L2_APP_ENV_H__
#define __DEM_L2_APP_ENV_H__

//???!!!forward declarations?
#include <Core/CoreServer.h>
#include <Time/TimeServer.h>
#include <Debug/DebugServer.h>
#include <IO/IOServer.h>
#include <Data/DataServer.h>
#include <Events/EventServer.h>
#include <Scripting/ScriptServer.h>
#include <Debug/DebugDraw.h>
//#include <Audio/AudioServer.h>
#include <Physics/PhysicsServer.h>
#include <Input/InputServer.h>
#include <Game/GameServer.h>
#include <AI/AIServer.h>
#include <UI/UIServer.h>
#include <Render/DisplayMode.h>
#include <System/OSWindow.h>
#include <Video/VideoServer.h>
#ifdef RegisterClass
#undef RegisterClass
#endif

//???need at all? redesign application framework!

// Environment class helps to setup and stores ptrs to engine subsystems. Use it to implement
// application classes faster. Init & shutdown processes are split into parts
// grouped by layers (either DEM LN or logical). It's done to allow user to perform
// his own initialization steps correctly.

// NOTE: this class doesn't handle L3 initialization since it belongs to L2. Derive from it into
// your L3 lib or application framework or initialize/release L3 systems manually in application.

namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}

namespace App
{

#define AppEnv App::CEnvironment::Instance()

class CEnvironment
{
protected:

	CString							AppName;
	CString							AppVersion;
	CString							AppVendor;
	CString							ProjDir;

	bool							AllowMultipleInstances;

	CString							WindowTitle;
	CString							IconName;
	//CDisplayMode					DisplayMode;
	Sys::POSWindow					MainWindow;

	Ptr<Time::CTimeServer>			TimeServer;
	Ptr<Debug::CDebugServer>		DebugServer;
	Ptr<IO::CIOServer>				IOServer;
	Ptr<Data::CDataServer>			DataServer;
	Ptr<Scripting::CScriptServer>	ScriptServer;
	Ptr<Events::CEventServer>		EventServer;
	//Ptr<Debug::CDebugDraw>			DD;
	Ptr<Physics::CPhysicsServer>	PhysicsServer;
	Ptr<Input::CInputServer>		InputServer;
	//Ptr<Audio::CAudioServer>		AudioServer;
	Ptr<Video::CVideoServer>		VideoServer;
	Ptr<Game::CGameServer>			GameServer;
	Ptr<AI::CAIServer>				AIServer;
	Ptr<UI::CUIServer>				UIServer;

	void RegisterAttributes();

public:

	CEnvironment(): AllowMultipleInstances(false) {}
	
	static CEnvironment* Instance() { static CEnvironment Singleton; return &Singleton; }

	bool InitCore();			//L0, Nebula, L1 kernel
	void ReleaseCore();
	bool InitEngine();			//L1 systems, L2 non-game systems
	void ReleaseEngine();
	bool InitGameSystem();		//L2 game system
	void ReleaseGameSystem();

	void					SetProjectDirectory(const CString& NewProjDir) { ProjDir = NewProjDir; }
	const CString&			GetProjectDirectory() const { return ProjDir; }
	void					SetWindowTitle(const char* pTitle) { WindowTitle = pTitle; }
	const CString&			GetWindowTitle() const { return WindowTitle; }
	void					SetWindowIcon(const char* pIconName) { IconName = pIconName; }
	const CString&			GetWindowIcon() const { return IconName; }
	//void					SetDisplayMode(const CDisplayMode& DispMode) { DisplayMode = DispMode; }
	//const CDisplayMode&		GetDisplayMode() const { return DisplayMode; }
	void					SetAppName(const CString& ApplicationName) { AppName = ApplicationName; }
	const CString&			GetAppName() const { return AppName; }
	void					SetAppVersion(const CString& Version) { AppVersion = Version; }
	const CString&			GetAppVersion() const { return AppVersion; }
	void					SetVendorName(const CString& Vendor) { AppVendor = Vendor; }
	const CString&			GetVendorName() const { return AppVendor; }
	void					SetAllowMultipleInstances(bool Allow) { AllowMultipleInstances = Allow; }
	bool					GetAllowMultipleInstances() const { return AllowMultipleInstances; }
};

}

#endif
