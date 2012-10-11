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
#include <Game/Mgr/EnvQueryManager.h>
#include <Physics/Prop/PropTransformable.h>
#include <Physics/Prop/PropAbstractPhysics.h>
#include <Physics/CharEntity.h>
#include <Physics/Composite.h>
#include <gfx2/ngfxserver2.h>

namespace App
{
__ImplementSingleton(App::CCIDEApp);

using namespace Properties;

CCIDEApp::CCIDEApp():
	ParentHwnd(NULL),
	pUIEventHandler(NULL),
	DenyEntityAboveGround(false),
	DenyEntityBelowGround(false)
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

	ClearSelectedEntities();
	CurrentEntity = NULL;
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

bool CCIDEApp::SelectEntity(Game::PEntity Entity)
{
	if (!Entity.isvalid()) FAIL;
	if (Entity.get_unsafe() != SelectedEntity.get_unsafe())
		SelectedEntity = Entity;
	OK;
}
//---------------------------------------------------------------------

void CCIDEApp::ClearSelectedEntities()
{
	SelectedEntity = NULL;
}
//---------------------------------------------------------------------

void CCIDEApp::ApplyGroundConstraints(const Game::CEntity& Entity, vector3& Position)
{
	if (!DenyEntityAboveGround && !DenyEntityBelowGround) return;

	int SelfPhysicsID;
	CPropAbstractPhysics* pPhysProp = Entity.FindProperty<CPropAbstractPhysics>();

	float LocalMinY = 0.f;
	if (pPhysProp)
	{
		Physics::CEntity* pPhysEnt = pPhysProp->GetPhysicsEntity();
		if (pPhysEnt)
		{
			SelfPhysicsID = pPhysEnt->GetUniqueID();
			
			bbox3 AABB;
			pPhysEnt->GetComposite()->GetAABB(AABB);

			//!!!THIS SHOULDN'T BE THERE! Tfm must be adjusted inside entity that fixes it.
			// I.e. AABB must be translated in GetAABB or smth.
			float PhysEntY = pPhysEnt->GetTransform().pos_component().y;
			float AABBPosY = AABB.center().y;

			LocalMinY -= (AABB.extents().y - AABBPosY + PhysEntY);

			//!!!tmp hack, need more general code!
			if (pPhysEnt->IsA(Physics::CCharEntity::RTTI))
				LocalMinY -= ((Physics::CCharEntity*)pPhysEnt)->Hover;
		}
		else SelfPhysicsID = -1;
	}
	else SelfPhysicsID = -1;

	CEnvInfo Info;
	EnvQueryMgr->GetEnvInfoAt(vector3(Position.x, Position.y + 500.f, Position.z), Info, 1000.f, SelfPhysicsID);
	
	float MinY = Position.y + LocalMinY;
	if ((DenyEntityAboveGround && MinY > Info.WorldHeight) ||
		(DenyEntityBelowGround && MinY < Info.WorldHeight))
		Position.y = Info.WorldHeight - LocalMinY;
}
//---------------------------------------------------------------------

} // namespace App