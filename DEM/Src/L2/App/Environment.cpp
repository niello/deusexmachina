#include "Environment.h"

#include <Game/EntityManager.h>
#include <Game/Mgr/FocusManager.h>
#include <Game/Mgr/StaticEnvManager.h>
#include <Scripting/EntityScriptObject.h>
#include <Core/Logger.h>

#undef DeleteFile

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
	CoreSrv->Open();

	DataServer.Create();

	n_new(Core::CLogger);
	CoreLogger->Open((AppName + " - " + AppVersion).CStr());

	if (!ProjDir.IsValid()) ProjDir = DataSrv->GetAssign("home");
	IOSrv->SetAssign("proj", ProjDir);

	nString AppData;
	AppData.Format("appdata:%s/%s", AppVendor.CStr(), AppName.CStr());
	IOSrv->SetAssign("appdata", IOSrv->ManglePath(AppData));

	OK;
}
//---------------------------------------------------------------------

void CEnvironment::ReleaseCore()
{
	DataServer = NULL;

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

	if (!Scripting::CEntityScriptObject::RegisterClass()) FAIL;
	EntityFct->Init();

	LoaderServer.Create();
	if (!LoaderServer->Open()) FAIL;

	RenderServer.Create();
	RenderServer->GetDisplay().SetWindowTitle(WindowTitle.CStr());
	RenderServer->GetDisplay().SetWindowIcon(IconName.CStr());
	RenderServer->GetDisplay().RequestDisplayMode(DisplayMode);
	if (!RenderServer->Open()) FAIL;

	SceneServer.Create();
	//???do it in Open()?
	Render::PFrameShader DefaultFrameShader = n_new(Render::CFrameShader);
	n_assert(DefaultFrameShader->Init(*DataSrv->LoadHRD("data:shaders/Default.hrd")));
	SceneServer->AddFrameShader(CStrID("Default"), DefaultFrameShader);
	SceneServer->SetScreenFrameShaderID(CStrID("Default"));

	DD.Create();
	if (!DD->Open()) FAIL;

	InputServer.Create();
	InputServer->Open();

	AudioServer.Create();
	AudioServer->Open();
	//nString TablePath = "data:tables/sound.xml";
	//if (IOSrv->FileExists(TablePath))
	//	AudioServer->OpenWaveBank(TablePath);
	//else n_printf("Warning: '%s' doesn't exist!\n", TablePath.CStr());

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

	if (VideoServer.IsValid() && VideoServer->IsOpen()) VideoServer->Close();
	VideoServer = NULL;

	if (AudioServer.IsValid() && AudioServer->IsOpen()) AudioServer->Close();
	AudioServer = NULL;

	DD->Close();
	DD = NULL;

	SceneServer = NULL;

	if (RenderServer.IsValid() && RenderServer->IsOpen()) RenderServer->Close();
	RenderServer = NULL;

	if (InputServer.IsValid() && InputServer->IsOpen()) InputServer->Close();
	InputServer = NULL;

	if (LoaderServer.IsValid() && LoaderServer->IsOpen()) LoaderServer->Close();
	LoaderServer = NULL;

	EntityFct->Release();

	DBServer = NULL;
	DebugServer = NULL;

	if (TimeServer.IsValid() && TimeServer->IsOpen()) TimeServer->Close();
	TimeServer = NULL;

	ScriptServer = NULL;
	EventManager = NULL;
}
//---------------------------------------------------------------------

bool CEnvironment::InitGameSystem()
{
	PhysicsServer.Create();
	PhysicsServer->Open();
	
	GameServer.Create();
	GameServer->Open();

	AIServer = AI::CAIServer::Create();

	Data::PParams ActTpls = DataSrv->LoadHRD("data:tables/AIActionTpls.hrd");
	if (ActTpls.IsValid())
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
	if (SOActTpls.IsValid())
		for (int i = 0; i < SOActTpls->GetCount(); ++i)
		{
			const Data::CParam& Prm = SOActTpls->Get(i);
			AISrv->AddSmartObjActionTpl(Prm.GetName(), *Prm.GetValue<Data::PParams>());
		}

	OK;
}
//---------------------------------------------------------------------

void CEnvironment::ReleaseGameSystem()
{
	AIServer = NULL;
	
	GameServer->Close();
	GameServer = NULL;

	PhysicsServer->Close();
	PhysicsServer = NULL;
}
//---------------------------------------------------------------------

}