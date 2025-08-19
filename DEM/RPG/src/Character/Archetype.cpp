#include "Archetype.h"
#include <Game/GameSession.h>

namespace DEM::RPG
{

void CArchetype::OnPostLoad(const Resources::CDataAssetLoaderHRD<DEM::RPG::CArchetype>& Loader)
{
	Loader.GetResourceManager().RegisterResource<CArchetype>(BaseArchetype);
	if (BaseArchetype) BaseArchetype->ValidateObject<CArchetype>();

	auto* pSession = static_cast<const Resources::CArchetypeLoader&>(Loader).GetSession();

	if (!Strength->FormulaStr.empty())
	{
		// Strength->FormulaStr -> Strength->Formula
	}

	// TODO:
	// maybe cache fallback pointers from base (need Ptr instead of unique_ptr then)
	// maybe cache Lua formulas in CNumericStatDefinition
	//???use reflection to iterate over CNumericStatDefinition fields?
	//???or use universal runtime map based on string IDs of stats from config?
}
//---------------------------------------------------------------------

}

namespace Resources
{

CArchetypeLoader::CArchetypeLoader(Resources::CResourceManager& ResourceManager, DEM::Game::CGameSession& Session)
	: CDataAssetLoaderHRD(ResourceManager)
	, _Session(&Session)
{
}
//---------------------------------------------------------------------

CArchetypeLoader::~CArchetypeLoader() = default;
//---------------------------------------------------------------------

}
