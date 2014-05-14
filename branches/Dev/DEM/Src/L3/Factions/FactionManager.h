#pragma once
#ifndef __DEM_L3_FACTION_MANAGER_H__
#define __DEM_L3_FACTION_MANAGER_H__

#include <Data/Singleton.h>
#include <Factions/Faction.h>
#include <Events/EventsFwd.h>
#include <Data/Dictionary.h>

// Faction manager manages faction list and cross-faction relations. Query it for example
// to know if one character/group is an ally or enemy of another character/group, even if
// groups are mixed of members of diffrent factions.
// NB: FactionA -> FactionB isn't always equal to FactionB -> FactionA

namespace RPG
{
#define FactionMgr RPG::CFactionManager::Instance()

class CFactionManager: public Core::CObject
{
	__DeclareClassNoFactory;
	__DeclareSingleton(CFactionManager);

private:

	CDict<CStrID, PFaction> Factions;

	DECLARE_EVENT_HANDLER(OnGameDescLoaded, OnGameDescLoaded);
	DECLARE_EVENT_HANDLER(OnGameSaving, OnGameSaving);

public:

	CFactionManager();
	~CFactionManager();

	CFaction*	GetFaction(CStrID ID) const;

	//!!!adding faction adds script object of faction class
};

inline CFaction* CFactionManager::GetFaction(CStrID ID) const
{
	int Idx = Factions.FindIndex(ID);
	return (Idx == INVALID_INDEX) ? NULL : Factions.ValueAt(Idx).GetUnsafe();
}
//---------------------------------------------------------------------

}

#endif
