#pragma once
#ifndef __DEM_L2_APP_ENV_H__
#define __DEM_L2_APP_ENV_H__

//???!!!forward declarations?
#include <Core/CoreServer.h>
#include <Time/TimeServer.h>
#include <Debug/DebugServer.h>
#include <Data/DataServer.h>
#include <Events/EventManager.h>
#include <Scripting/ScriptServer.h>
#include <DB/DBServer.h>
#include <Loading/LoaderServer.h>
#include <Gfx/GfxServer.h>
#include <Render/RenderServer.h>
#include <Scene/SceneServer.h>
#include <Audio/AudioServer.h>
#include <Video/VideoServer.h>
#include <Physics/PhysicsServer.h>
#include <Input/InputServer.h>
#include <Game/GameServer.h>
#include <AI/AIServer.h>
#include <UI/UIServer.h>
#include <Render/DisplayMode.h>

// Environment class helps to setup and stores ptrs to engine subsystems. Use it to implement
// application classes faster. Init & shutdown processes are split into parts
// grouped by layers (either DEM LN or logical). It's done to allow user to perform
// his own initialization steps correctly.

// NOTE: this class doesn't handle L3 initialization since it belongs to L2. Derive from it into
// your L3 lib or application framework or initialize/release L3 systems manually in application.

class nResourceServer;

namespace App
{

#define AppEnv App::CEnvironment::Instance()

class CEnvironment
{
protected:

	nString							AppName;
	nString							AppVersion;
	nString							AppVendor;
	nString							ProjDir;

	bool							AllowMultipleInstances;

	nString							WindowTitle;
	nString							IconName;
	CDisplayMode					DisplayMode;
	nString							RenderPath;
	nString							FeatureSet;

	nRef<nResourceServer>			refResourceServer;

	Ptr<Time::CTimeServer>			TimeServer;
	Ptr<Debug::CDebugServer>		DebugServer;
	Ptr<Data::CDataServer>			DataServer;
	Ptr<Scripting::CScriptServer>	ScriptServer;
	Ptr<Events::CEventManager>		EventManager;
	Ptr<DB::CDBServer>				DBServer;
	Ptr<Loading::CLoaderServer>		LoaderServer;
	Ptr<Graphics::CGfxServer>		GfxServer;
	Ptr<Render::CRenderServer>		RenderServer;
	Ptr<Scene::CSceneServer>		SceneServer;
	Ptr<Physics::CPhysicsServer>	PhysicsServer;
	Ptr<Input::CInputServer>		InputServer;
	Ptr<Audio::CAudioServer>		AudioServer;
	Ptr<Video::CVideoServer>		VideoServer;
	//Ptr<VFX::Server>				VFXServer;
	Ptr<Game::CGameServer>			GameServer;
	Ptr<AI::CAIServer>				AIServer;
	Ptr<UI::CUIServer>				UIServer;

	void RegisterAttributes();

public:

	CEnvironment();
	virtual ~CEnvironment(); //???virtual?
	
	static CEnvironment* Instance() { static CEnvironment Singleton; return &Singleton; }

	bool InitCore();			//L0, Nebula, L1 kernel
	void ReleaseCore();
	bool InitEngine();			//L1 systems, L2 non-game systems
	void ReleaseEngine();
	bool InitGameSystem();		//L2 game system
	void ReleaseGameSystem();

	void					SetProjectDirectory(const nString& NewProjDir) { ProjDir = NewProjDir; }
	const nString&			GetProjectDirectory() const { return ProjDir; }
	void					SetWindowTitle(const char* pTitle) { WindowTitle = pTitle; }
	const nString&			GetWindowTitle() const { return WindowTitle; }
	void					SetWindowIcon(const char* pIconName) { IconName = pIconName; }
	const nString&			GetWindowIcon() const { return IconName; }
	void					SetDisplayMode(const CDisplayMode& DispMode) { DisplayMode = DispMode; }
	const CDisplayMode&		GetDisplayMode() const { return DisplayMode; }
	void					SetRenderPath(const nString& RP) { RenderPath = RP; }
	const nString&			GetRenderPath() const { return RenderPath; }
	void					SetFeatureSet(const nString& FS) { FeatureSet = FS; }
	const nString&			GetFeatureSet() const { return FeatureSet; }
	void					SetAppName(const nString& ApplicationName) { AppName = ApplicationName; }
	const nString&			GetAppName() const { return AppName; }
	void					SetAppVersion(const nString& Version) { AppVersion = Version; }
	const nString&			GetAppVersion() const { return AppVersion; }
	void					SetVendorName(const nString& Vendor) { AppVendor = Vendor; }
	const nString&			GetVendorName() const { return AppVendor; }
	void					SetAllowMultipleInstances(bool Allow) { AllowMultipleInstances = Allow; }
	bool					GetAllowMultipleInstances() const { return AllowMultipleInstances; }
};

}

#endif
