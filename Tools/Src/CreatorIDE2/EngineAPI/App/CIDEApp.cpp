#include "CIDEApp.h"

#include "AppStateEditor.h"
#include <App/Environment.h>
#include <Game/EntityLoader.h>
#include <Physics/PropPhysics.h>
#include <SI/SI_L1.h>
#include <SI/SI_L2.h>
#include <SI/SI_L3.h>
#include "../Prop/PropEditorCamera.h"

#include <time.h>

//nNebulaUsePackage(ncterrain2lib);

namespace App
{
//__ImplementSingleton(App::CCIDEApp);
PCIDEApp AppInst = NULL;

CCIDEApp::CCIDEApp():
	ParentHwnd(NULL),
	MouseCB(NULL),
	DataPathCB(NULL),
	TransformMode(false),
	LimitToGround(false),
	SnapToGround(false)
{
	//__ConstructSingleton;
}
//---------------------------------------------------------------------

CCIDEApp::~CCIDEApp()
{
	//__DestructSingleton;
}
//---------------------------------------------------------------------

void CCIDEApp::SetupDisplayMode()
{
	CDisplayMode Mode;
	Mode.PosX = 0;
	Mode.PosY = 0;
	Mode.Width = 800;
	Mode.Height = 600;
	//Mode.VSync = false;
	AppEnv->SetDisplayMode(Mode);

	//!!!SET FULLSCREEN / WINDOWED! //???to display mode?

	CString WindowTitle = GetVendorName() + " - " + GetAppName() + " - " + GetAppVersion();
	AppEnv->SetWindowTitle(WindowTitle.CStr());

	AppEnv->SetWindowIcon("Icon");
}
//---------------------------------------------------------------------

bool CCIDEApp::Open()
{
	AppEnv->SetAppName(GetAppName());
	AppEnv->SetAppVersion(GetAppVersion());
	AppEnv->SetVendorName(GetVendorName());

	SetupDisplayMode();

	// Old:
	//this->SetupFromDefaults();
	//this->SetupFromProfile();
	//this->SetupFromCmdLineArgs();
	
	if (!AppEnv->InitCore())
	{
		AppEnv->ReleaseCore();
		FAIL;
	}
	
	IOSrv->ReleaseMemoryCB = ReleaseMemoryCB;
	IOSrv->DataPathCB = DataPathCB;
	ReleaseMemoryCB = NULL;
	DataPathCB = NULL;

	CString Home = IOSrv->ManglePath("home:");
	CString Proj = IOSrv->ManglePath("proj:");
	CString Data = Proj + "/Project";
	CString Export = Proj + "/export";

	// Path may be redefined by an external editor, so we need to query it after assignement
	IOSrv->SetAssign("data", Data);
	Data = IOSrv->ManglePath("data:");
	IOSrv->SetAssign("export", Export);
	Export = IOSrv->ManglePath("export:");

	IOSrv->SetAssign("renderpath", Home + "/Shaders/");
	IOSrv->SetAssign("scripts", Home + "/Scripts/");
	IOSrv->SetAssign("physics", Export + "/physics/");
	IOSrv->SetAssign("meshes", Export + "/meshes/");
	IOSrv->SetAssign("textures", Export + "/textures/");
	IOSrv->SetAssign("anims", Export + "/anims/");
	IOSrv->SetAssign("gfxlib", Export + "/gfxlib/");
	IOSrv->SetAssign("db", Export + "/db/");
	IOSrv->SetAssign("game", Export + "/game/");
	IOSrv->SetAssign("sound", Export + "/audio/");
	IOSrv->SetAssign("cegui", Export + "/cegui/");

	CoreSrv->SetGlobal("parent_hwnd", (int)ParentHwnd);

	if (!AppEnv->InitEngine())
	{
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}

	//nGfxServer2::Instance()->SetCursorVisibility(nGfxServer2::System);

	//RegisterAttributes();
	SI::RegisterGlobals();
	SI::RegisterEventServer();
	SI::RegisterEntityManager();

	InputSrv->SetContextLayout(CStrID("Debug"), CStrID("Debug"));
	InputSrv->SetContextLayout(CStrID("Game"), CStrID("Game"));
	InputSrv->SetContextLayout(CStrID("Editor"), CStrID("Editor"));

	InputSrv->EnableContext(CStrID("Debug"));

	n_printf("Setup input - OK\n");

	//EntityFct->SetDefaultLoader(Loading::CEntityLoader::Create());

	// Editor must load all static entities separately, so we comment it
	//EntityFct->SetLoader(CStrID("StaticEnv"), Loading::CEnvironmentLoader::Create());

	// Init UI //!!!fix texture path! add cegui:imagesets/ here! set resgroup for it to imagesets
	////!!!to HRD params! data/cfg/UI.hrd
	//UISystem->LoadFont("DejaVuSans-8.font");
	//UISystem->LoadFont("DejaVuSans-10.font");
	//UISystem->LoadFont("DejaVuSans-14.font");
	//UISystem->LoadScheme("TaharezLook.scheme");
	//UISystem->SetDefaultMouseCursor("TaharezLook", "MouseArrow");

	if (!AppEnv->InitGameSystem())
	{
		AppEnv->ReleaseGameSystem();
		AppEnv->ReleaseEngine();
		AppEnv->ReleaseCore();
		FAIL;
	}

	//init gameplay
	QuestSystem = Story::CQuestManager::Instance();
	DlgSystem = Story::CDialogueManager::Instance(); //???init into Game::Server as manager?
	ItemManager = Items::CItemManager::Instance(); //???init into Game::Server as manager?

	SI::RegisterQuestSystem();

	FSM.AddStateHandler(n_new(CAppStateEditor(CStrID("Editor"), this)));
	FSM.Init(CStrID("Editor"));

	/*DB::PValueTable CameraTable = DB::CValueTable::Create();
	CameraTable->AddColumn(Attr::GUID);
	CameraTable->AddColumn(Attr::FieldOfView);
	CameraTable->AddColumn(Attr::Transform);*/

	//EditorCamera = EntityFct->CreateTmpEntity(CStrID("__EditorCamera"), CStrID("Dummy"), CameraTable, CameraTable->AddRow());
	//EntityFct->AttachProperty<Properties::CPropEditorCamera>(*EditorCamera);
	//EntityFct->AttachProperty<Properties::CPropInput>(*EditorCamera);
	//EntityFct->AttachProperty<Properties::CPropTransformable>(*EditorCamera);
	//matrix44 Tfm;
	////!!!bad hardcode!
	//Tfm.lookatRh(vector3(0.f, -3.f, -2.f), vector3(0.f, 1.f, 0.f));
	//Tfm.translate(vector3(300.f, 145.f, 300.f));
	//EditorCamera->Set<matrix44>(Attr::Transform, Tfm);

	OK;
}
//---------------------------------------------------------------------

bool CCIDEApp::AdvanceFrame()
{
	return FSM.Advance();
}
//---------------------------------------------------------------------

void CCIDEApp::Close()
{
	CurrentEntity = NULL;
	CurrentTfmEntity = NULL;
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

void CCIDEApp::SetDataPathCB(IO::CDataPathCallback Cb, IO::CReleaseMemoryCallback ReleaseCb)
{
	if(Data::CDataServer::HasInstance())
	{
		IOSrv->ReleaseMemoryCB = ReleaseCb;
		IOSrv->DataPathCB = Cb;
	}
	else
	{	
		DataPathCB = Cb;
		ReleaseMemoryCB = ReleaseCb;
	}
}
//---------------------------------------------------------------------

void CCIDEApp::ApplyGroundConstraints(const Game::CEntity& Entity, vector3& Position)
{
	if (!DenyEntityAboveGround && !DenyEntityBelowGround) return;

	int SelfPhysicsID = -1;
	Prop::CPropPhysics* pPhysProp = Entity.GetProperty<Prop::CPropPhysics>();

	float LocalMinY = 0.f;
	if (pPhysProp)
	{
		Game::CEntity* pPhysEnt = pPhysProp->GetEntity();
		if (pPhysEnt)
		{
			//SelfPhysicsID = pPhysProp->GetUID();

			CAABB AABB;
			pPhysProp->GetAABB(AABB);

			////!!!THIS SHOULDN'T BE THERE! Tfm must be adjusted inside entity that fixes it.
			//// I.e. AABB must be translated in GetAABB or smth.
			//float PhysEntY = pPhysProp->GetTransform().pos_component().y;
			//float AABBPosY = AABB.center().y;

			//LocalMinY -= (AABB.extents().y - AABBPosY + PhysEntY);

			////!!!tmp hack, need more general code!
			//if (pPhysEnt->IsA(Physics::CCharEntity::RTTI))
			//	LocalMinY -= ((Physics::CCharEntity*)pPhysEnt)->Hover;
		}
		else SelfPhysicsID = -1;
	}
	else SelfPhysicsID = -1;

	/*CEnvInfo Info;
	EnvQueryMgr->GetEnvInfoAt(vector3(Position.x, Position.y + 500.f, Position.z), Info, 1000.f, SelfPhysicsID);

	float MinY = Position.y + LocalMinY;
	if ((DenyEntityAboveGround && MinY > Info.WorldHeight) ||
		(DenyEntityBelowGround && MinY < Info.WorldHeight))
		Position.y = Info.WorldHeight - LocalMinY;*/
}
//---------------------------------------------------------------------

} // namespace App