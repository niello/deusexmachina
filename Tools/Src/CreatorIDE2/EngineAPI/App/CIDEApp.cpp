#include "CIDEApp.h"

#include "AppStateEditor.h"
#include <App/Environment.h>
#include <Loading/EntityFactory.h>
#include <Loading/EntityLoader.h>
#include <Loading/EnvironmentLoader.h>
#include <SI/SI_L1.h>
#include <SI/SI_L2.h>
#include <SI/SI_L3.h>
#include "../Prop/PropEditorCamera.h"
#include <Input/Prop/PropInput.h>
#include <Physics/Prop/PropTransformable.h>
#include <DB/StdAttrs.h>
#include <Game/Mgr/FocusManager.h>

#include <gfx2/ngfxserver2.h>

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

	nString WindowTitle = GetVendorName() + " - " + GetAppName() + " - " + GetAppVersion();
	AppEnv->SetWindowTitle(WindowTitle.Get());

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
	
	DataSrv->ReleaseMemoryCB = ReleaseMemoryCB;
	DataSrv->DataPathCB = DataPathCB;
	ReleaseMemoryCB = NULL;
	DataPathCB = NULL;

	nString Home = DataSrv->ManglePath("home:");
	nString Proj = DataSrv->ManglePath("proj:");
	nString Data = Proj + "/Project";
	nString Export = Proj + "/export";

	// Path may be redefined by an external editor, so we need to query it after assignement
	DataSrv->SetAssign("data", Data);
	Data = DataSrv->ManglePath("data:");
	DataSrv->SetAssign("export", Export);
	Export = DataSrv->ManglePath("export:");

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

	nGfxServer2::Instance()->SetCursorVisibility(nGfxServer2::System);

	//RegisterAttributes();
	SI::RegisterGlobals();
	SI::RegisterEventManager();
	SI::RegisterEntityManager();

	InputSrv->SetContextLayout(CStrID("Debug"), CStrID("Debug"));
	InputSrv->SetContextLayout(CStrID("Game"), CStrID("Game"));
	InputSrv->SetContextLayout(CStrID("Editor"), CStrID("Editor"));

	InputSrv->EnableContext(CStrID("Debug"));

	n_printf("Setup input - OK\n");

	EntityFct->SetDefaultLoader(Loading::CEntityLoader::Create());

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
	QuestSystem = Story::CQuestSystem::Create();
	DlgSystem = Story::CDlgSystem::Create(); //???init into Game::Server as manager?
	ItemManager = Items::CItemManager::Create(); //???init into Game::Server as manager?

	SI::RegisterQuestSystem();

	FSM.AddStateHandler(n_new(CAppStateEditor(CStrID("Editor"), this)));
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

void CCIDEApp::SetDataPathCB(Data::CDataPathCallback Cb, Data::CReleaseMemoryCallback ReleaseCb)
{
	if(Data::CDataServer::HasInstance())
	{
		DataSrv->ReleaseMemoryCB = ReleaseCb;
		DataSrv->DataPathCB = Cb;
	}
	else
	{	
		DataPathCB = Cb;
		ReleaseMemoryCB = ReleaseCb;
	}
}
//---------------------------------------------------------------------

/*
#include "tools/ncmdlineargs.h"

void
CApplication::SetupFromCmdLineArgs()
{
    //// setup optional startup savegame or level paths
    //this->SetStartupSavegame(this->cmdLineArgs.GetStringArg("-loadgame", 0));
    //this->SetStartupLevel(this->cmdLineArgs.GetStringArg("-level", 0));
    //this->SetWorldDb(this->cmdLineArgs.GetStringArg("-db", 0));

    // setup display mode
    nDisplayMode2 mode = AppEnv->GetDisplayMode();
    if (this->cmdLineArgs.HasArg("-fullscreen"))
    {
        if (this->cmdLineArgs.GetBoolArg("-fullscreen") | this->GetForceFullscreen())
        {
            mode.SetType(nDisplayMode2::Fullscreen);
        }
        else
        {
            mode.SetType(nDisplayMode2::Windowed);
        }
    }
    mode.SetXPos(this->cmdLineArgs.GetIntArg("-x", mode.GetXPos()));
    mode.SetYPos(this->cmdLineArgs.GetIntArg("-y", mode.GetYPos()));
    mode.SetWidth(this->cmdLineArgs.GetIntArg("-w", mode.GetWidth()));
    mode.SetHeight(this->cmdLineArgs.GetIntArg("-h", mode.GetHeight()));
    mode.SetAntiAliasSamples(this->cmdLineArgs.GetIntArg("-aa", mode.GetAntiAliasSamples()));
    AppEnv->SetDisplayMode(mode);

    // set project directory override
    nString projDirArg = this->cmdLineArgs.GetStringArg("-projdir", 0);
    if (projDirArg.IsValid())
    {
        AppEnv->SetProjectDirectory(projDirArg);
    }

    // set feature set override
    nString featureSetArg = this->cmdLineArgs.GetStringArg("-featureset", 0);
    if (featureSetArg.IsValid())
    {
        AppEnv->SetFeatureSet(featureSetArg);
    }

    // set render path override
    nString renderPathArg = this->cmdLineArgs.GetStringArg("-renderpath", 0);
    if (renderPathArg.IsValid())
    {
        AppEnv->SetRenderPath(renderPathArg);
    }
}
*/

} // namespace App