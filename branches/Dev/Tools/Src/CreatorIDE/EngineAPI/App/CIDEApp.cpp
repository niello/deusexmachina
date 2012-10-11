#include "CIDEApp.h"

#include "AppStateEditor.h"
#include <App/Environment.h>
#include <App/CSharpUIEventHandler.h>
#include <Loading/EntityFactory.h>
#include <Loading/EntityLoader.h>
#include <SI/SI_L1.h>
#include <SI/SI_L2.h>
#include <SI/SI_L3.h>
#include <Prop/PropEditorCamera.h>
#include <Input/Prop/PropInput.h>
#include <Physics/Prop/PropTransformable.h>
#include <gfx2/ngfxserver2.h>

namespace App
{
__ImplementSingleton(App::CCIDEApp);

CCIDEApp::CCIDEApp():
	ParentHwnd(NULL),
	pUIEventHandler(NULL),
	TransformMode(false),
	LimitToGround(false),
	SnapToGround(false)
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CCIDEApp::~CCIDEApp()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CCIDEApp::Open()
{
	nString WindowTitle = GetVendorName() + " - " + GetAppName() + " - " + GetAppVersion();

	AppEnv->SetVendorName(GetVendorName());
	AppEnv->SetAppName(GetAppName());
	AppEnv->SetAppVersion(GetAppVersion());
	AppEnv->SetDisplayMode(CDisplayMode(0, 0, 800, 600, false));
	AppEnv->SetWindowTitle(WindowTitle.Get());
	AppEnv->SetWindowIcon("Icon");

	//!!!SET FULLSCREEN / WINDOWED! //???to display mode?

	if (!AppEnv->InitCore())
	{
		AppEnv->ReleaseCore();
		FAIL;
	}

	//!!!TO HRD SETTINGS!
	nString Home = DataSrv->ManglePath("home:");
	nString Proj = DataSrv->ManglePath("proj:");
	nString Data = Proj + "/Content/Project";
	nString Export = Proj + "/Content/export";
	DataSrv->SetAssign("data", Data);
	DataSrv->SetAssign("export", Export);
	DataSrv->SetAssign("renderpath", Home + "/Shaders/");
	DataSrv->SetAssign("scripts", Home + "/Scripts/");
	DataSrv->SetAssign("physics", Export + "/physics/");
	DataSrv->SetAssign("meshes", Export + "/meshes/");
	DataSrv->SetAssign("textures", Export + "/textures/");
	DataSrv->SetAssign("anims", Export + "/anims/");
	DataSrv->SetAssign("gfxlib", Export + "/gfxlib/");
	DataSrv->SetAssign("db", Export + "/db/");
	DataSrv->SetAssign("game", Export + "/game/");
	DataSrv->SetAssign("sound", Export + "/audio/");
	DataSrv->SetAssign("cegui", Export + "/cegui/");

	CoreSrv->SetGlobal("parent_hwnd", (int)ParentHwnd);

	if (!AppEnv->InitEngine())
	{
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}

	DbgSrv->AllowUI(false);

	nGfxServer2::Instance()->SetCursorVisibility(nGfxServer2::System);

	SI::RegisterGlobals();
	SI::RegisterEventManager();
	SI::RegisterEntityManager();

	InputSrv->SetContextLayout(CStrID("Debug"), CStrID("Debug"));
	InputSrv->SetContextLayout(CStrID("Game"), CStrID("Game"));
	InputSrv->SetContextLayout(CStrID("Editor"), CStrID("Editor"));

	InputSrv->EnableContext(CStrID("Debug"));

	n_printf("Setup input - OK\n");

	// Editor must load all static entities separately, so we don't enable CEnvironmentLoader
	EntityFct->SetDefaultLoader(Loading::CEntityLoader::Create());

	if (!AppEnv->InitGameSystem())
	{
		AppEnv->ReleaseGameSystem();
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}

	// Init gameplay
	QuestSystem = Story::CQuestSystem::Create();
	DlgSystem = Story::CDlgSystem::Create(); //???init into Game::Server as manager?
	ItemManager = Items::CItemManager::Create(); //???init into Game::Server as manager?

	SI::RegisterQuestSystem();

	FSM.AddStateHandler(n_new(CAppStateEditor(CStrID("Editor"))));
	FSM.Init(CStrID("Editor"));

	DB::PValueTable CameraTable = DB::CValueTable::Create();
	CameraTable->AddColumn(Attr::GUID);
	CameraTable->AddColumn(Attr::FieldOfView);
	CameraTable->AddColumn(Attr::Transform);

	EditorCamera = EntityFct->CreateTmpEntity(CStrID("__EditorCamera"), CStrID("Dummy"), CameraTable, CameraTable->AddRow());
	EntityFct->AttachProperty<Properties::CPropEditorCamera>(*EditorCamera);
	EntityFct->AttachProperty<Properties::CPropInput>(*EditorCamera);
	EntityFct->AttachProperty<Properties::CPropTransformable>(*EditorCamera);
	matrix44 Tfm;
	//!!!bad hardcode!
	Tfm.lookatRh(vector3(0.f, -3.f, -2.f), vector3(0.f, 1.f, 0.f));
	Tfm.translate(vector3(300.f, 145.f, 300.f));
	EditorCamera->Set<matrix44>(Attr::Transform, Tfm);

	pUIEventHandler = n_new(CCSharpUIEventHandler);

	OK;
}
//---------------------------------------------------------------------

void CCIDEApp::Close()
{
	n_delete(pUIEventHandler);
	pUIEventHandler = NULL;

	CurrentEntity = NULL;
	SelectedEntity = NULL;
	EditorCamera = NULL;

	AttrDescs = NULL;

	FSM.Clear();

	DlgSystem = NULL;
	QuestSystem = NULL;
	ItemManager = NULL;

	AppEnv->ReleaseGameSystem();
	AppEnv->ReleaseEngine();
	AppEnv->ReleaseCore();
}
//---------------------------------------------------------------------

bool CCIDEApp::SetEditorTool(LPCSTR Name)
{
	CAppStateEditor* pState = (CAppStateEditor*)FSM.FindStateHandlerByID(CStrID("Editor"));
	return pState && pState->SetTool(Name);
}
//---------------------------------------------------------------------

} // namespace App