#include "SocialManager.h"

namespace DEM::RPG
{

CSocialManager::CSocialManager(Game::CGameSession& Owner)
	: _Session(Owner)
{
}
//---------------------------------------------------------------------

const CFactionInfo* CSocialManager::FindFaction(CStrID ID) const
{
	auto It = _Factions.find(ID);
	return (It != _Factions.cend()) ? &It->second : nullptr;
}
//---------------------------------------------------------------------

U32 CSocialManager::GetPartyTrait(CStrID ID) const
{
	auto It = _PartyTraits.find(ID);
	return (It != _PartyTraits.cend()) ? It->second : 0;
}
//---------------------------------------------------------------------

}
