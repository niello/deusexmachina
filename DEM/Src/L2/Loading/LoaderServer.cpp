#include "LoaderServer.h"

#include <Events/EventManager.h>
#include <Game/Mgr/EntityManager.h>
#include <Game/Mgr/StaticEnvManager.h>
#include <Loading/EntityFactory.h>
#include <Time/TimeServer.h>
#include <Physics/PhysicsServer.h>
#include <Physics/Level.h>
#include <Gfx/GfxServer.h>
#include <Scene/SceneServer.h>
#include <AI/AIServer.h>
#include <Game/GameServer.h>
#include <DB/Database.h>
#include <DB/DBServer.h>
#include <Data/DataServer.h>
#include <Scripting/ScriptObject.h>

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
ImplementRTTI(Loading::CLoaderServer, Core::CRefCounted);
ImplementFactory(Loading::CLoaderServer);
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
	DataSrv->CreateDirectory("appdata:profiles/default");
	nString Path = GetDatabasePath();
	if (!DataSrv->FileExists(Path)) Path = nString("export:db/") + GameDBName + ".db3";
	if (!OpenGameDB(Path)) FAIL;

	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CLoaderServer::Close()
{
	n_assert(_IsOpen);

	if (GameDB.isvalid())
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

bool CLoaderServer::OpenStaticDB(const nString& SrcDBPath)
{
	n_assert(!StaticDB.isvalid());
	if (!DataSrv->FileExists(SrcDBPath)) FAIL;

	StaticDB.Create();
	StaticDB->URI = SrcDBPath;
	StaticDB->SetExclusiveMode(true);
#ifdef _EDITOR
	StaticDB->AccessMode = CDatabase::DBAM_ReadWriteExisting;
#else
	StaticDB->AccessMode = CDatabase::DBAM_ReadOnly;
#endif
	StaticDB->SetIgnoreUnknownColumns(true);
	StaticDB->SetInMemoryDatabase(UseInMemoryDB);
	if (!StaticDB->Open())
	{
		nString Path = DataSrv->ManglePath(SrcDBPath);
		n_error("Failed to open static database '%s'!\nError: %s", Path.Get(), StaticDB->GetError().Get());
		FAIL;
	}
	OK;
}
//---------------------------------------------------------------------

bool CLoaderServer::OpenGameDB(const nString& SrcDBPath)
{
	if (!DataSrv->FileExists(SrcDBPath)) FAIL;

	if (GameDB.isvalid())
	{
		EventMgr->FireEvent(CStrID("OnGameDBClose"));
		GameDB->Close();
		n_assert(GameDB->GetRefCount() == 1);
	}

	//!!!if (!UseInMemoryDB) {

	nString DBPath = GetDatabasePath();
	//if (DataSrv->FileExists(DBPath)) DataSrv->DeleteFile(DBPath);
	if (SrcDBPath != DBPath)
		n_assert(DataSrv->CopyFile(SrcDBPath, DBPath));

	if (!GameDB.isvalid())
	{
		GameDB.Create();
		GameDB->URI = DBPath;
		GameDB->SetExclusiveMode(true);
		GameDB->AccessMode = CDatabase::DBAM_ReadWriteExisting;
		GameDB->SetIgnoreUnknownColumns(true);
		GameDB->SetInMemoryDatabase(UseInMemoryDB);
	}

	if (!GameDB->Open())
	{
		nString Path = DataSrv->ManglePath(DBPath);
		n_error("Failed to open game database '%s'!\nError: %s", Path.Get(), GameDB->GetError().Get());
		FAIL;
	}

	LoadGlobalAttributes(); //???for empty level too?

	OK;
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
	
	if (Globals.GetAttrs().Size() < 1) return;

	DB::PTable Tbl = DB::CTable::Create();
	Tbl->SetName("_Globals");
	for (int i = 0; i < Globals.GetAttrs().Size(); i++)
		Tbl->AddColumn(Globals.GetAttrs().KeyAtIndex(i));
	GameDB->AddTable(Tbl);

	DB::PDataset DS = Tbl->CreateDataset();
	DS->AddColumnsFromTable();
	DS->AddRow();
	for (int i = 0; i < Globals.GetAttrs().Size(); i++)
		DS->SetValue(Globals.GetAttrs().KeyAtIndex(i), Globals.GetAttrs().ValueAtIndex(i));
	DS->CommitChanges();
}
//---------------------------------------------------------------------

bool CLoaderServer::LoadLevel(const nString& LevelName)
{
	n_assert(!EntityMgr->GetNumEntities());

	CStrID LevelID(LevelName.Get());

	SetCurrentLevel(LevelName);

	int QuadTreeDepth;

	DB::PDataset DS;

	if (LevelName.IsValid())
	{
		DS = GameDB->GetTable("Levels")->CreateDataset();
		DS->AddColumnsFromTable();
		DS->SetWhereClause("GUID='" + LevelName + "'");
		DS->PerformQuery();
		
		if (DS->GetValueTable()->GetRowCount() != 1)
		{
			n_error("Loading::CLoaderServer::LoadLevel(): Level '%s' %s in world database!",
				LevelName.Get(), DS->GetValueTable()->GetRowCount() ? "has more than one record" : "not found");
			FAIL;
		}

		DS->SetRowIndex(0);
		const vector4& Center = DS->Get<vector4>(Attr::Center);
		const vector4& Extents = DS->Get<vector4>(Attr::Extents);
		LevelBox.set(vector3(Center.x, Center.y, Center.z), vector3(Extents.x, Extents.y, Extents.z));
		QuadTreeDepth = 5;

		LevelScript = n_new(Scripting::CScriptObject(("Level_" + LevelName).Get()));
		LevelScript->Init(); // No special class
		LevelScript->LoadScriptFile("scripts:Levels/" + LevelName + ".lua");
	}
	else
	{
		LevelID = CStrID("Temp");
		LevelBox = EmptyLevelBox;
		QuadTreeDepth = 3;
	}

	PhysicsSrv->SetLevel(Physics::CLevel::Create());

	Ptr<Graphics::CLevel> GfxLevel;
	GfxLevel.Create();
	GfxLevel->Init(LevelBox, QuadTreeDepth);
	GfxSrv->SetLevel(GfxLevel);

	n_assert(SceneSrv->CreateScene(LevelID, LevelBox, true));

	n_assert(AISrv->SetupLevel(LevelBox));

	if (LevelName.IsValid())
	{
		const nString& NavMeshRsrc = DS->Get<nString>(Attr::NavMesh);
		if (NavMeshRsrc.IsValid())
		{
			nString NavMeshFileName;
			NavMeshFileName.Format("export:Nav/%s.nm", NavMeshRsrc.Get());
			AISrv->GetLevel()->LoadNavMesh(NavMeshFileName);
		}

		DS = NULL;

		EntityFct->LoadEntityInstances(LevelName);

		// OnLoad is here to give Entities and Managers a chance to do some additional
		// initialization after all world state has been loaded form the database.
		PParams P = n_new(CParams);
		P->Set(CStrID("DB"), (PVOID)GameDB.get_unsafe());
		EventMgr->FireEvent(CStrID("OnLoadBefore"), P);
		EventMgr->FireEvent(CStrID("OnLoad"), P);
		EventMgr->FireEvent(CStrID("OnLoadAfter"), P);
	}
	else
	{
		EntityFct->InitEmptyInstanceDatasets();
		TimeSrv->ResetAll();
	}

	GameSrv->Start();

	PParams P = n_new(CParams);
	P->Set(CStrID("Level"), LevelName);
	EventMgr->FireEvent(CStrID("OnLevelLoaded"), P);

	OK;
}
//---------------------------------------------------------------------

void CLoaderServer::UnloadLevel()
{
	//!!!correctly handle no level loaded case!
	//!!!unload AI level!
	GameSrv->Stop();
	StaticEnvMgr->ClearStaticEnv();
	EntityMgr->RemoveAllEntities();
	SceneSrv->RemoveScene(SceneSrv->GetCurrentSceneID());
	GfxSrv->SetLevel(NULL);
	PhysicsSrv->SetLevel(NULL);
	LevelBox.vmin = vector3::Zero;
	LevelBox.vmax = vector3::Zero;
	LevelScript = NULL;
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
	P->Set(CStrID("DB"), (PVOID)GameDB.get_unsafe());
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

nString CLoaderServer::GetStartupLevel() const
{
	//???fail hard if no startup level?
	return HasGlobal(Attr::StartupLevel) ? GetGlobal<nString>(Attr::StartupLevel) : NULL;
}
//---------------------------------------------------------------------

void CLoaderServer::SetCurrentLevel(const nString& LevelName)
{
	//???check does level exist?
	SetGlobal<nString>(Attr::CurrentLevel, LevelName);
}
//---------------------------------------------------------------------

nString CLoaderServer::GetCurrentLevel() const
{
	return HasGlobal(Attr::CurrentLevel) ? GetGlobal<nString>(Attr::CurrentLevel) : NULL;
}
//---------------------------------------------------------------------

bool CLoaderServer::IsRandomEncounterLevel(const nString& LevelName)
{
	n_error("Loading::CLoaderServer::IsRandomEncounterLevel() - Not implemented yet!");
	return true;
}
//---------------------------------------------------------------------

} // namespace Loading

///**
//    This opens the database in New Game mode: an original database will be
//    copied into the user's profile directory into a Current Game State
//    database and opened.
//*/
//bool
//DbServer::OpenNewGame(const nString& profileURI, const nString& dbURI)
//{
//    n_assert(profileURI.IsValid());
//
//    // make sure we're not open
//    if (IsGameDatabaseOpen())
//    {
//        CloseGameDatabase();
//    }
//    
//    if (!UseInMemoryDB)
//    {        
//        // make sure the target directory exists
//        DataSrv->CreateDirectory(profileURI);
//        
//        // delete current database
//        if (ioServer->FileExists(dbURI))
//        {
//            bool dbDeleted = ioServer->DeleteFile(dbURI);
//            n_assert(dbDeleted);
//        }
//        nString journalURI = dbURI + nString("-journal");
//        // delete current database journalfile
//        if (ioServer->FileExists(journalURI))
//        {
//            bool dbDeleted = ioServer->DeleteFile(journalURI);
//            n_assert(dbDeleted);
//        }
//        bool dbCopied = ioServer->CopyFile("export:db/game.db4", dbURI);
//        n_assert(dbCopied);
//    }
//
//    // open the copied database file
//    bool dbOpened = OpenGameDatabase(dbURI);
//    return dbOpened;
//}
//
////------------------------------------------------------------------------------
///**
//    This opens the database in Continue Game mode: the current game database
//    in the user profile directory will simply be opened.
//*/
//bool
//DbServer::OpenContinueGame(const nString& profileURI)
//{
//    n_assert(profileURI.IsValid());
//
//    // make sure we're not open
//    if (IsGameDatabaseOpen())
//    {
//        CloseGameDatabase();
//    }
//    
//    // open the current game database
//    bool dbOpened = OpenGameDatabase(profileURI);
//    return dbOpened;
//}
//
////------------------------------------------------------------------------------
///**
//    Return true if a current game database exists.
//*/
//bool
//DbServer::CurrentGameExists(const nString& profileURI) const
//{
//    n_assert(profileURI.IsValid());
//    return DataSrv->FileExists(profileURI);
//}
//
////------------------------------------------------------------------------------
///**
//    This opens the database in Load Game mode. This will overwrite the
//    current game database with a save game database and open this as
//    usual.
//*/
//bool
//DbServer::OpenLoadGame(const nString& profileURI, const nString& dbURI, const nString& saveGameURI)
//{
//    n_assert(profileURI.IsValid());
//    
//    // make sure we're not open
//    if (IsGameDatabaseOpen())
//    {
//        CloseGameDatabase();
//    }
//    
//    if (!UseInMemoryDB)
//    {
//        // make sure the target directory exists        
//        DataSrv->CreateDirectory(profileURI);
//    }
//    
//    // check if save game exists
//    if (!DataSrv->FileExists(saveGameURI))
//    {
//        return false;
//    }
//    
//    if (!UseInMemoryDB)
//    {
//        // delete current database
//        if (ioServer->FileExists(dbURI))
//        {
//            bool dbDeleted = ioServer->DeleteFile(dbURI);
//            n_assert(dbDeleted);
//        }
//        bool dbCopied = ioServer->CopyFile(saveGameURI, dbURI);
//        n_assert(dbCopied);
//    }
//    
//    // open the copied database file
//    bool dbOpened = OpenGameDatabase(dbURI);
//    return dbOpened;
//}
//
////------------------------------------------------------------------------------
///**
//    This creates a new save game by making a copy of the current world
//    database into the savegame directory. If a savegame of that exists,
//    it will be overwritten.
//*/
//bool
//DbServer::CreateSaveGame(const nString& profileURI, const nString& dbURI, const nString& saveGameURI)
//{
//    n_assert2(!UseInMemoryDB, "TODO: Savegame from memory db not implemented yet!");
//    
//    // make sure the target directory exists
//    DataSrv->CreateDirectory(profileURI);
//
//    // check if current world database exists
//    if (!DataSrv->FileExists(dbURI))
//    {
//        return false;
//    }
//
//    // delete save game file if already exists
//    if (DataSrv->FileExists(saveGameURI))
//    {
//        bool saveGameDeleted = ioServer->DeleteFile(saveGameURI);
//        n_assert(saveGameDeleted);
//    }
//    else
//    {
//        nString saveGamePath = saveGameURI.ExtractDirName();        
//        DataSrv->CreateDirectory(saveGamePath);
//    }
//    bool saveGameCopied = ioServer->CopyFile(dbURI, saveGameURI);
//    n_assert(saveGameCopied);
//    return true;
//}
//
