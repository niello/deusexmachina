#include "Environment.h"

#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/EnvQueryManager.h>
#include <Game/Mgr/FocusManager.h>
#include <Game/Mgr/StaticEnvManager.h>
#include <Loading/EntityFactory.h>
#include <Scripting/EntityScriptObject.h>
#include <Core/Logger.h>

#include <gfx2/ngfxserver2.h>
#include <resource/nresourceserver.h>

#undef DeleteFile

nNebulaUsePackage(nnebula);
nNebulaUsePackage(ndirect3d9);

namespace App
{

CEnvironment::CEnvironment(): AllowMultipleInstances(false)
{
}
//---------------------------------------------------------------------

CEnvironment::~CEnvironment()
{
}
//---------------------------------------------------------------------

bool CEnvironment::InitCore()
{
	//!!!core server can check is app already running inside the Open method!
	n_new(Core::CCoreServer());
	nKernelServer::Instance()->AddPackage(nnebula);
	nKernelServer::Instance()->AddPackage(ndirect3d9);
	CoreSrv->Open();

	refResourceServer = n_new(nResourceServer);
	refResourceServer->AddRef();

	DataServer.Create();

	n_new(Core::CLogger);
	CoreLogger->Open((AppName + " - " + AppVersion).Get());

	if (!ProjDir.IsValid()) ProjDir = DataSrv->GetAssign("home");
	DataSrv->SetAssign("proj", ProjDir);

	nString AppData;
	AppData.Format("appdata:%s/%s", AppVendor.Get(), AppName.Get());
	DataSrv->SetAssign("appdata", DataSrv->ManglePath(AppData));

	OK;
}
//---------------------------------------------------------------------

void CEnvironment::ReleaseCore()
{
	DataServer = NULL;

	if (refResourceServer.isvalid()) refResourceServer->Release();

	CoreLogger->Close();
	n_delete(CoreLogger);

	CoreSrv->Close();
	n_delete(CoreSrv);
}
//---------------------------------------------------------------------

bool CEnvironment::InitEngine()
{
	EventManager.Create();
	ScriptServer.Create();

	TimeServer.Create();
	TimeServer->Open();

	DebugServer.Create();
	DebugServer->RegisterPlugin(CStrID("Console"), "Debug::CLuaConsole", "Console.layout");
	DebugServer->RegisterPlugin(CStrID("Watcher"), "Debug::CWatcherWindow", "Watcher.layout");

	DBServer.Create();
	RegisterAttributes();

	RsrcServer.Create();

	if (!Scripting::CEntityScriptObject::RegisterClass()) FAIL;
	EntityFct->Init();

	LoaderServer.Create();
	if (!LoaderServer->Open()) FAIL;

	GfxServer.Create();
	GfxServer->WindowTitle = WindowTitle;
	GfxServer->WindowIcon = IconName;
	GfxServer->DisplayMode = DisplayMode;
	GfxServer->RenderPath = RenderPath; //???store here?
	GfxServer->FeatureSet = FeatureSet; //???store here?
	GfxServer->Open();

	Renderer.Create();
	//!!!Renderer->Open();

	SceneServer.Create();

	InputServer.Create();
	InputServer->Open();

	AudioServer.Create();
	AudioServer->Open();
	//nString TablePath = "data:tables/sound.xml";
	//if (DataSrv->FileExists(TablePath))
	//	AudioServer->OpenWaveBank(TablePath);
	//else n_printf("Warning: '%s' doesn't exist!\n", TablePath.Get());

	VideoServer.Create();
	VideoServer->Open();
	
	UIServer.Create();
	DbgSrv->AllowUI(true);

	OK;
}
//---------------------------------------------------------------------

void CEnvironment::ReleaseEngine()
{
	DbgSrv->AllowUI(false);
	UIServer = NULL;

	if (VideoServer.isvalid() && VideoServer->IsOpen()) VideoServer->Close();
	VideoServer = NULL;

	if (AudioServer.isvalid() && AudioServer->IsOpen()) AudioServer->Close();
	AudioServer = NULL;

	SceneServer = NULL;

	//!!!if (Renderer.isvalid() && Renderer->IsOpen()) Renderer->Close();
	Renderer = NULL;

	if (GfxServer.isvalid() && GfxServer->IsOpen()) GfxServer->Close();
	GfxServer = NULL;

	if (InputServer.isvalid() && InputServer->IsOpen()) InputServer->Close();
	InputServer = NULL;

	if (LoaderServer.isvalid() && LoaderServer->IsOpen()) LoaderServer->Close();
	LoaderServer = NULL;

	EntityFct->Release();

	RsrcServer = NULL;

	DBServer = NULL;
	DebugServer = NULL;

	if (TimeServer.isvalid() && TimeServer->IsOpen()) TimeServer->Close();
	TimeServer = NULL;

	ScriptServer = NULL;
	EventManager = NULL;
}
//---------------------------------------------------------------------

bool CEnvironment::InitGameSystem()
{
	PhysicsServer = Physics::CPhysicsServer::Create();
	PhysicsServer->Open();
	
	GameServer = Game::CGameServer::Create();
	GameServer->Open();

	GameServer->AttachManager(Game::CEntityManager::Create());
	GameServer->AttachManager(Game::CStaticEnvManager::Create());
	GameServer->AttachManager(Game::CEnvQueryManager::Create());
	GameServer->AttachManager(Game::CFocusManager::Create());

	AIServer = AI::CAIServer::Create();

	Data::PParams ActTpls = DataSrv->LoadHRD("data:tables/AIActionTpls.hrd");
	if (ActTpls.isvalid())
	{
		for (int i = 0; i < ActTpls->GetCount(); ++i)
		{
			const Data::CParam& Prm = ActTpls->Get(i);
			AISrv->GetPlanner().RegisterActionTpl(Prm.GetName().CStr(), Prm.GetValue<Data::PParams>());
		}
		AISrv->GetPlanner().EndActionTpls();
	}

	// Smart object actions
	//???Load from DB?
	Data::PParams SOActTpls = DataSrv->LoadHRD("data:tables/AISOActionTpls.hrd");
	if (SOActTpls.isvalid())
		for (int i = 0; i < SOActTpls->GetCount(); ++i)
		{
			const Data::CParam& Prm = SOActTpls->Get(i);
			AISrv->AddSmartObjActionTpl(Prm.GetName(), *Prm.GetValue<Data::PParams>());
		}
	
	GfxServer->EntityTimeSrc = TimeServer->GetTimeSource(CStrID("Game"));

	OK;
}
//---------------------------------------------------------------------

void CEnvironment::ReleaseGameSystem()
{
	AIServer = NULL;
	
	GameServer->RemoveAllManagers();
	GameServer->Close();
	GameServer = NULL;

	PhysicsServer->Close();
	PhysicsServer = NULL;
}
//---------------------------------------------------------------------

}