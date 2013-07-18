#pragma once
#ifndef __DEM_L3_WORLD_MANAGER_H__
#define __DEM_L3_WORLD_MANAGER_H__

#include <Core/RefCounted.h>
#include <Core/Singleton.h>
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

class CWorldManager: public Core::CRefCounted
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CWorldManager);

private:

	DECLARE_EVENT_HANDLER(OnGameDescLoaded, OnGameDescLoaded);
	DECLARE_EVENT_HANDLER(OnGameSaving, OnGameSaving);

public:

	CWorldManager();
	~CWorldManager();

	bool	MakeTransition(const CArray<CStrID>& EntityIDs, CStrID DepartureLevelID, CStrID DestLevelID, CStrID DestMarkerID, bool UnloadDepartureLevel);
};

}

#endif
