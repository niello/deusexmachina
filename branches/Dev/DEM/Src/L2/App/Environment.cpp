#include "Environment.h"

#include <Scripting/EntityScriptObject.h>

namespace App
{

bool CEnvironment::InitEngine()
{
	EventServer = n_new(Events::CEventServer);
	ScriptServer = n_new(Scripting::CScriptServer);

	TimeServer = n_new(Time::CTimeServer);
	TimeServer->Open();

	DebugServer = n_new(Debug::CDebugServer);
	DebugServer->RegisterPlugin(CStrID("Console"), "Debug::CLuaConsole", "Console.layout");
	DebugServer->RegisterPlugin(CStrID("Watcher"), "Debug::CWatcherWindow", "Watcher.layout");

	if (!Scripting::CEntityScriptObject::RegisterClass()) FAIL;

	//???do it in RenderServer->Open()?
	//Render::PFrameShader DefaultFrameShader = n_new(Render::CFrameShader);
	//n_assert(DefaultFrameShader->Init(*DataSrv->LoadPRM("Shaders:Default.prm")));
	//RenderServer->AddFrameShader(CStrID("Default"), DefaultFrameShader);
	//RenderServer->SetScreenFrameShaderID(CStrID("Default"));

	DD = n_new(Debug::CDebugDraw);
	if (!DD->Open()) FAIL;

	InputServer = n_new(Input::CInputServer);
	InputServer->Open();

	//AudioServer = n_new(Audio::CAudioServer);
	//AudioServer->Open();

	VideoServer = n_new(Video::CVideoServer);
	VideoServer->Open();
	
	//UIServer = n_new(UI::CUIServer);
	//DbgSrv->AllowUI(true);

	OK;
}
//---------------------------------------------------------------------

void CEnvironment::ReleaseEngine()
{
	DbgSrv->AllowUI(false);
	UIServer = NULL;

	if (VideoServer.IsValid() && VideoServer->IsOpen()) VideoServer->Close();
	VideoServer = NULL;

	//if (AudioServer.IsValid() && AudioServer->IsOpen()) AudioServer->Close();
	//AudioServer = NULL;

	DD->Close();
	DD = NULL;

	//if (RenderServer.IsValid() && RenderServer->IsOpen()) RenderServer->Close();
	//RenderServer = NULL;

	if (InputServer.IsValid() && InputServer->IsOpen()) InputServer->Close();
	InputServer = NULL;

	//if (LoaderServer.IsValid() && LoaderServer->IsOpen()) LoaderServer->Close();
	//LoaderServer = NULL;

	//DBServer = NULL;
	DebugServer = NULL;

	if (TimeServer.IsValid() && TimeServer->IsOpen()) TimeServer->Close();
	TimeServer = NULL;

	ScriptServer = NULL;
	EventServer = NULL;
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
			AISrv->AddSmartAction(Prm.GetName(), *Prm.GetValue<Data::PParams>());
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