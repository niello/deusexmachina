#include "SocialManager.h"
#include <Data/SerializeToParams.h>

namespace DEM::RPG
{

CSocialManager::CSocialManager(Game::CGameSession& Owner)
	: _Session(Owner)
{
}
//---------------------------------------------------------------------

void CSocialManager::LoadFactions(const Data::PParams& Desc)
{
	DEM::ParamsFormat::DeserializeAppend(Data::CData(Desc), _Factions);
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
