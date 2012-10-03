#pragma once
#ifndef __DEM_L2_LOADER_SERVER_H__
#define __DEM_L2_LOADER_SERVER_H__

#include <Core/RefCounted.h>

#include <Data/StringID.h>
#include <Data/Singleton.h>
#include <Loading/UserProfile.h>
#include <Loading/CharacterProfile.h>
#include <DB/AttrSet.h>
#include <util/ndictionary.h>
#include <Data/DataServer.h>
#include <mathlib/bbox.h>

// The Loading::CLoaderServer is the central object of the loader subsystem
// Usually you don't work directly with the Loading subsystem, but instead
// use higher level classes like the Game::SetupManager and
// Game::CSaveGameManager.
// Based on mangalore Loading::CLoaderServer (C) 2006 RadonLabs GmbH

namespace DB
{
	typedef Ptr<class CDatabase> PDatabase;
}

namespace Scripting
{
	typedef Ptr<class CScriptObject> PScriptObject;
}

namespace Loading
{
using namespace Profiles;

#define LoaderSrv Loading::CLoaderServer::Instance()

class CLoaderServer: public Core::CRefCounted
{
	DeclareRTTI;
	DeclareFactory(CLoaderServer);
	__DeclareSingleton(CLoaderServer);

private:

	bool						_IsOpen;
	bbox3						EmptyLevelBox;

	nString						GameDBName; //???to profile? or game-wide parameter?
	Ptr<UserProfile>			UserPrf;
	Ptr<CharacterProfile>		ChrPrf;

	DB::PDatabase				StaticDB;
	DB::PDatabase				GameDB;
	DB::CAttrSet				Globals;
	bool						UseInMemoryDB;

	bbox3						LevelBox; // Level info cached
	Scripting::PScriptObject	LevelScript;

    bool	OpenStaticDB(const nString& SrcDBPath);
	bool	OpenGameDB(const nString& SrcDBPath);
	void	LoadGlobalAttributes();
	void	SaveGlobalAttributes(); 

	//bool OpenNewGame(const nString& profileURI, const nString& dbURI);
	//bool OpenContinueGame(const nString& profileURI);
	//bool OpenLoadGame(const nString& profileURI, const nString& dbURI, const nString& saveGameURI);
	////bool CreateSaveGame(const nString& profileURI, const nString& dbURI, const nString& saveGameURI);
	//bool CurrentGameExists(const nString& profileURI) const;

public:

	CLoaderServer();
	virtual ~CLoaderServer();

	bool				Open();
	void				Close();
	bool				IsOpen() const { return _IsOpen; }

	bool				LoadLevel(const nString& LevelName);
	bool				LoadEmptyLevel() { return LoadLevel(NULL); }
	void				UnloadLevel();

	void				CommitChangesToDB();
	
	bool				NewGame(const nString& StartupLevel = NULL);
	bool				ContinueGame();
	bool				LoadGame(const nString& SaveGameName);
	bool				SaveGame(const nString& SaveGameName);
	
	//!!!EnumSavedGames(/*???profile?*/)! or Profile->EnumSavedGames()
	//EnumSavedGames() to get list of saves for curr profile

	//???need createprofile functions? only to create different profile types through virtual calls!
	UserProfile*		CreateUserProfile() const { return UserProfile::Create(); }
	void				SetUserProfile(UserProfile* pPrf) { UserPrf = pPrf; }
	UserProfile*		GetUserProfile() const { return UserPrf; }
	nString				GetSaveGameDirectory() const;
	nString				GetDatabasePath() const;
	nString				GetSaveGamePath(const nString& SaveGameName) const;

	CharacterProfile*	CreateCharacterProfile() const { return CharacterProfile::Create(); }
	void				SetCharacterProfile(CharacterProfile* pPrf) { return ChrPrf = pPrf; }
	CharacterProfile*	GetCharacterProfile() const { return ChrPrf; }

	bool				HasGlobal(DB::CAttrID AttrID) const { return Globals.HasAttr(AttrID); }
	template<class T>
	void				SetGlobal(DB::CAttrID AttrID, const T& Value) { Globals.SetAttr(AttrID, Value); }
	template<class T>
	const T&			GetGlobal(DB::CAttrID AttrID) const { return Globals.GetAttr(AttrID).GetValue<T>(); }

	//!!!only for CEntityFactory now!
	DB::CDatabase*		GetStaticDB() { return StaticDB.get_unsafe(); }
	DB::CDatabase*		GetGameDB() { return GameDB.get_unsafe(); }

	void				SetGameDBName(const nString& DBName) { GameDBName = (DBName.IsValid()) ? DBName : "game"; }
	const nString&		GetGameDBName() const { return GameDBName; }
	nString				GetStartupLevel() const;
	void				SetCurrentLevel(const nString& LevelName);
	nString				GetCurrentLevel() const;
	const bbox3&		GetCurrentLevelBox() const { return LevelBox; }
	bool				IsRandomEncounterLevel(const nString& LevelName);
	void				SetEmptyLevelDimensions(const bbox3& Box) { EmptyLevelBox = Box; }
	const bbox3&		GetEmptyLevelDimensions() const { return EmptyLevelBox; }
};

RegisterFactory(CLoaderServer);

//???!!!pre-create this strings in profile when it's loaded?!

// Returns the path to the current world database.
inline nString CLoaderServer::GetDatabasePath() const
{
	return UserPrf->GetProfileDirectory() + "/" + GameDBName + ".db3";
}
//---------------------------------------------------------------------

// Returns the path to the user's savegame directory (inside the profile
// directory) using the Nebula2 filesystem path conventions.
inline nString CLoaderServer::GetSaveGameDirectory() const
{
	return UserPrf->GetProfileDirectory() + "/save";
}
//---------------------------------------------------------------------

// Get the complete filename to a savegame file.
inline nString CLoaderServer::GetSaveGamePath(const nString& SaveGameName) const
{
	return GetSaveGameDirectory() + "/" + SaveGameName + ".db3";
}
//---------------------------------------------------------------------

// This method starts a new game by creating a copy of the initial
// world database into the current user profile's directory. 
// The given StartupLevel will be loaded.
inline bool CLoaderServer::NewGame(const nString& StartupLevel)
{
	DataSrv->CreateDirectory(UserPrf->GetProfileDirectory());
	if (!OpenGameDB(nString("export:db/") + GameDBName + ".db3")) FAIL;
	return LoadLevel(StartupLevel.IsValid() ? StartupLevel : GetStartupLevel());
}
//---------------------------------------------------------------------

// This method continues the game from the last known state (the existing
// world database file in the user profile's directory, created by NewGame().
inline bool CLoaderServer::ContinueGame()
{
	if (!OpenGameDB(GetDatabasePath())) FAIL;
	return LoadLevel(GetCurrentLevel());
}
//---------------------------------------------------------------------

// Load a saved game. This will overwrite the current world database
// with the saved game database file.
inline bool CLoaderServer::LoadGame(const nString& SaveGameName)
{
	if (!OpenGameDB(GetSaveGamePath(SaveGameName))) FAIL;
	return LoadLevel(GetCurrentLevel());
}
//---------------------------------------------------------------------

}

#endif