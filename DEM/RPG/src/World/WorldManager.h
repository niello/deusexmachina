#pragma once
#ifndef __DEM_L3_WORLD_MANAGER_H__
#define __DEM_L3_WORLD_MANAGER_H__

#include <Core/Object.h>
#include <Data/Singleton.h>
#include <Data/StringID.h>
#include <Events/EventsFwd.h>

// This class manages all aspects of the game world in its pseudo-realistic aspects.
// It includes traveling between different locations and sublocations, world time,
// calendar, that can differ from the real-world one, weather at different places in
// the world, aspects of astronomy, and maybe some probabilities to meet random animals
// or to found random plants.

namespace RPG
{
#define WorldMgr RPG::CWorldManager::Instance()

class CWorldManager: public DEM::Core::CObject
{
	RTTI_CLASS_DECL(RPG::CWorldManager, DEM::Core::CObject);
	__DeclareSingleton(CWorldManager);

private:

	DECLARE_EVENT_HANDLER(OnGameDescLoaded, OnGameDescLoaded);
	DECLARE_EVENT_HANDLER(OnGameSaving, OnGameSaving);

public:

	CWorldManager();
	~CWorldManager();

	//???to L2 GameSrv?
	bool	MakeTransition(const std::vector<CStrID>& EntityIDs, CStrID LevelID, CStrID MarkerID, bool UnloadAllLevels);
};

}

#endif
