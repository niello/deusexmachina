#pragma once
#ifndef __DEM_L2_LOADER_SERVER_H__
#define __DEM_L2_LOADER_SERVER_H__

#include <Core/RefCounted.h>

#include <Data/StringID.h>
#include <Core/Singleton.h>
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
#define LoaderSrv Loading::CLoaderServer::Instance()

class CLoaderServer: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CLoaderServer);

private:

	bool						_IsOpen;
	bbox3						EmptyLevelBox;

	nString						GameDBName; //???to profile? or game-wide parameter?

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

	void				CommitChangesToDB();
	
	bool				NewGame(const nString& StartupLevel = NULL);
	bool				ContinueGame();
	bool				LoadGame(const nString& SaveGameName);
	bool				SaveGame(const nString& SaveGameName);
	
	//!!!EnumSavedGames(/*???profile?*/)! or Profile->EnumSavedGames()
	//EnumSavedGames() to get list of saves for curr profile

	nString				GetSaveGameDirectory() const;
	nString				GetDatabasePath() const;
	nString				GetSaveGamePath(const nString& SaveGameName) const;

	bool				HasGlobal(DB::CAttrID AttrID) const { return Globals.HasAttr(AttrID); }
	template<class T>
	void				SetGlobal(DB::CAttrID AttrID, const T& Value) { Globals.SetAttr(AttrID, Value); }
	template<class T>
	const T&			GetGlobal(DB::CAttrID AttrID) const { return Globals.GetAttr(AttrID).GetValue<T>(); }

	//!!!only for CEntityFactory now!
	DB::CDatabase*		GetStaticDB() { return StaticDB.GetUnsafe(); }
	DB::CDatabase*		GetGameDB() { return GameDB.GetUnsafe(); }

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

//???!!!pre-create this strings in profile when it's loaded?!

// Returns the path to the current world database.
inline nString CLoaderServer::GetDatabasePath() const
{
	return "appdata:profiles/default/" + GameDBName + ".db3";
}
//---------------------------------------------------------------------

// Returns the path to the user's savegame directory (inside the profile
// directory) using the Nebula2 filesystem path conventions.
inline nString CLoaderServer::GetSaveGameDirectory() const
{
	return "appdata:profiles/default/save";
}
//---------------------------------------------------------------------

// Get the complete filename to a savegame file.
inline nString CLoaderServer::GetSaveGamePath(const nString& SaveGameName) const
{
	return GetSaveGameDirectory() + "/" + SaveGameName + ".db3";
}
//---------------------------------------------------------------------

}

#endif