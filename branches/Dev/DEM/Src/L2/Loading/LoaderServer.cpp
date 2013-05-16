#include "LoaderServer.h"

#include <Events/EventManager.h>
#include <Game/EntityManager.h>
#include <Game/Mgr/StaticEnvManager.h>
#include <Time/TimeServer.h>
#include <Physics/PhysicsServer.h>
#include <Physics/PhysicsLevel.h>
#include <Scene/SceneServer.h>
#include <AI/AIServer.h>
#include <Game/GameServer.h>
#include <DB/Database.h>
#include <DB/DBServer.h>
#include <Data/DataServer.h>
#include <Scripting/ScriptObject.h>

//!!!try to remove when frame shader code will be finished! Some Win32 includes have reached this place.
#undef CreateDirectory
#undef CopyFile

namespace Attr
{
	DefineString(CurrentLevel);
	DefineString(StartupLevel);
	DefineString(LevelID);
	DefineVector3(Center);
	DefineVector3(Extents);
	DefineString(NavMesh);
}

BEGIN_ATTRS_REGISTRATION(LoaderServer)
	RegisterString(CurrentLevel, ReadWrite);
	RegisterString(StartupLevel, ReadOnly);
	RegisterString(LevelID, ReadWrite);
	RegisterVector3(Center, ReadOnly);
	RegisterVector3(Extents, ReadOnly);
	RegisterString(NavMesh, ReadOnly);
END_ATTRS_REGISTRATION

namespace Loading
{
__ImplementClassNoFactory(Loading::CLoaderServer, Core::CRefCounted);
__ImplementClass(Loading::CLoaderServer);
__ImplementSingleton(Loading::CLoaderServer);

CLoaderServer::CLoaderServer():
	_IsOpen(false),
	GameDBName("game"),
	UseInMemoryDB(false),
	EmptyLevelBox(vector3(0.0f, 0.0f, 0.0f), vector3(500.0f, 100.0f, 500.0f))
{
	__ConstructSingleton;
}
//---------------------------------------------------------------------

CLoaderServer::~CLoaderServer()
{
	__DestructSingleton;
}
//---------------------------------------------------------------------

bool CLoaderServer::Open()
{
	n_assert(!_IsOpen);

	if (!OpenStaticDB("export:db/static.db3")) FAIL;

	EntityFct->LoadEntityTemplates();

	//???or use lazy init in loadlevel? in what db should store levels?
	IOSrv->CreateDirectory("appdata:profiles/default");
	nString Path = GetDatabasePath();
	if (!IOSrv->FileExists(Path)) Path = nString("export:db/") + GameDBName + ".db3";
	if (!OpenGameDB(Path)) FAIL;

	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CLoaderServer::Close()
{
	n_assert(_IsOpen);

	if (GameDB.IsValid())
	{
		EventMgr->FireEvent(CStrID("OnGameDBClose"));
		EntityFct->UnloadEntityInstances();
		GameDB->Close();
		n_assert(GameDB->GetRefCount() == 1);
		GameDB = NULL;
	}
	
	EntityFct->UnloadEntityTemplates();
	StaticDB->Close();
	n_assert(StaticDB->GetRefCount() == 1);
	StaticDB = NULL;
	
	_IsOpen = false;
}
//---------------------------------------------------------------------

void CLoaderServer::LoadGlobalAttributes()
{
	Globals.Clear();
	int Idx = GameDB->FindTableIndex("_Globals");
	if (Idx != INVALID_INDEX)
	{
		DB::PDataset DS = GameDB->GetTable(Idx)->CreateDataset();
		DS->AddColumnsFromTable();
		DS->PerformQuery();
		DB::CValueTable* Vals = DS->GetValueTable();
		n_assert(Vals->GetRowCount() == 1);
		for (int i = 0; i < Vals->GetNumColumns(); i++)
		{
			CData Val;
			Vals->GetValue(i, 0, Val);
			Globals.SetAttr(Vals->GetColumnID(i), Val);
		}
	}
}
//---------------------------------------------------------------------

void CLoaderServer::SaveGlobalAttributes()
{
	// We delete table instead of truncating it cause some columns may be were removed
	if (GameDB->HasTable("_Globals"))
		GameDB->DeleteTable("_Globals");
	
	if (Globals.GetAttrs().GetCount() < 1) return;

	DB::PTable Tbl = DB::CTable::Create();
	Tbl->SetName("_Globals");
	for (int i = 0; i < Globals.GetAttrs().GetCount(); i++)
		Tbl->AddColumn(Globals.GetAttrs().KeyAtIndex(i));
	GameDB->AddTable(Tbl);

	DB::PDataset DS = Tbl->CreateDataset();
	DS->AddColumnsFromTable();
	DS->AddRow();
	for (int i = 0; i < Globals.GetAttrs().GetCount(); i++)
		DS->SetValue(Globals.GetAttrs().KeyAtIndex(i), Globals.GetAttrs().ValueAtIndex(i));
	DS->CommitChanges();
}
//---------------------------------------------------------------------

// This method flushes all unsaved state to the world database. Servers & managers
// can use -Before & -After events to prepare for saving managed objects.
void CLoaderServer::CommitChangesToDB()
{
	if (GetCurrentLevel().IsEmpty())
	{
#ifdef _EDITOR
		GameDB->BeginTransaction();
		SaveGlobalAttributes();
		EntityFct->CommitChangesToDB(); // To save templates
		GameDB->EndTransaction();
#endif
		return;
	}

	GameDB->BeginTransaction();
	PParams P = n_new(CParams);
	P->Set(CStrID("DB"), (PVOID)GameDB.GetUnsafe());
	EventMgr->FireEvent(CStrID("OnSaveBefore"), P);
	EventMgr->FireEvent(CStrID("OnSave"), P);
	EventMgr->FireEvent(CStrID("OnSaveAfter"), P);
	SaveGlobalAttributes();
	EntityFct->CommitChangesToDB();
	GameDB->EndTransaction();
}
//---------------------------------------------------------------------

bool CLoaderServer::SaveGame(const nString& SaveGameName)
{
	CommitChangesToDB();
	n_assert(DataSrv->CopyFile(GetDatabasePath(), GetSaveGamePath(SaveGameName)));
	OK;
}
//---------------------------------------------------------------------

}