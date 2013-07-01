#include "Environment.h"

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

	IOServer = n_new(IO::CIOServer);
	DataServer = n_new(Data::CDataServer);

	if (!ProjDir.IsValid()) ProjDir = IOSrv->GetAssign("Home");
	IOSrv->SetAssign("Proj", ProjDir);

	Data::PParams PathList = DataSrv->LoadHRD("Proj:PathList.hrd", false);
	if (PathList.IsValid())
		for (int i = 0; i < PathList->GetCount(); ++i)
			IOSrv->SetAssign(PathList->Get(i).GetName().CStr(), IOSrv->ManglePath(PathList->Get<nString>(i)));

	n_new(Core::CLogger);
	CoreLogger->Open((AppName + " - " + AppVersion).CStr());

	nString AppData;
	AppData.Format("AppData:%s/%s", AppVendor.CStr(), AppName.CStr());
	IOSrv->SetAssign("AppData", IOSrv->ManglePath(AppData));

	OK;
}
//---------------------------------------------------------------------

void CEnvironment::ReleaseCore()
{
	DataServer = NULL;
	IOServer = NULL;

	CoreLogger->Close();
	n_delete(CoreLogger);

	CoreSrv->Close();
	n_delete(CoreSrv);
}
//---------------------------------------------------------------------

bool CEnvironment::InitEngine()
{
	EventManager = n_new(Events::CEventManager);
	ScriptServer = n_new(Scripting::CScriptServer);

	TimeServer = n_new(Time::CTimeServer);
	TimeServer->Open();

	DebugServer = n_new(Debug::CDebugServer);
	DebugServer->RegisterPlugin(CStrID("Console"), "Debug::CLuaConsole", "Console.layout");
	DebugServer->RegisterPlugin(CStrID("Watcher"), "Debug::CWatcherWindow", "Watcher.layout");

	if (!Scripting::CEntityScriptObject::RegisterClass()) FAIL;

	RenderServer = n_new(Render::CRenderServer);
	RenderServer->GetDisplay().SetWindowTitle(WindowTitle.CStr());
	RenderServer->GetDisplay().SetWindowIcon(IconName.CStr());
	RenderServer->GetDisplay().RequestDisplayMode(DisplayMode);
	if (!RenderServer->Open()) FAIL;

	//???manage frame shaders in a RenderSrv?
	SceneServer = n_new(Scene::CSceneServer);
	//???do it in Open()?
	Render::PFrameShader DefaultFrameShader = n_new(Render::CFrameShader);
	n_assert(DefaultFrameShader->Init(*DataSrv->LoadPRM("Shaders:Default.prm")));
	SceneServer->AddFrameShader(CStrID("Default"), DefaultFrameShader);
	SceneServer->SetScreenFrameShaderID(CStrID("Default"));

	DD = n_new(Render::CDebugDraw);
	if (!DD->Open()) FAIL;

	InputServer = n_new(Input::CInputServer);
	InputServer->Open();

	AudioServer = n_new(Audio::CAudioServer);
	AudioServer->Open();
	//nString TablePath = "data:tables/sound.xml";
	//if (IOSrv->FileExists(TablePath))
	//	AudioServer->OpenWaveBank(TablePath);
	//else n_printf("Warning: '%s' doesn't exist!\n", TablePath.CStr());

	VideoServer = n_new(Video::CVideoServer);
	VideoServer->Open();
	
	UIServer = n_new(UI::CUIServer);
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

	//if (LoaderServer.IsValid() && LoaderServer->IsOpen()) LoaderServer->Close();
	//LoaderServer = NULL;

	//DBServer = NULL;
	DebugServer = NULL;

	if (TimeServer.IsValid() && TimeServer->IsOpen()) TimeServer->Close();
	TimeServer = NULL;

	ScriptServer = NULL;
	EventManager = NULL;
}
//---------------------------------------------------------------------

bool CEnvironment::InitGameSystem()
{
	PhysicsServer = n_new(Physics::CPhysicsServer);
	PhysicsServer->Open();

	GameServer = n_new(Game::CGameServer);
	GameServer->Open();

	AIServer = n_new(AI::CAIServer);

	// Actor action templates
	Data::PParams ActTpls = DataSrv->LoadPRM("AI:AIActionTpls.prm");
	if (ActTpls.IsValid())
	{
		for (int i = 0; i < ActTpls->GetCount(); ++i)
		{
			const Data::CParam& Prm = ActTpls->Get(i);
			AISrv->GetPlanner().RegisterActionTpl(Prm.GetName().CStr(), Prm.GetValue<Data::PParams>());
		}
		AISrv->GetPlanner().EndActionTpls();
	}

	// Smart object action templates
	Data::PParams SOActTpls = DataSrv->LoadPRM("AI:AISOActionTpls.prm");
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