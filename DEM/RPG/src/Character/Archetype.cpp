#include "Archetype.h"
#include <Game/GameSession.h>

namespace DEM::RPG
{

static void LoadFormulaScript(CNumericStatDefinition* pDesc, sol::state& ScriptState, std::string_view StatName)
{
	if (pDesc && !pDesc->FormulaStr.empty())
	{
		// TODO: load() loads the chunk as a function but can't pass named arguments there (except by "x = ..."). What is the best way?
		auto Result = ScriptState.script("return function(Sheet) return {} end"_format(pDesc->FormulaStr));
		if (Result.valid())
			pDesc->Formula = Result;
		else
			::Sys::Error("CArchetype::OnPostLoad() > error loading stat '{}', formula '{}': {}"_format(StatName, pDesc->FormulaStr, Result.get<sol::error>().what()));
	}
}
//---------------------------------------------------------------------

void CArchetype::OnPostLoad(const Resources::CDataAssetLoaderHRD<DEM::RPG::CArchetype>& Loader)
{
	Loader.GetResourceManager().RegisterResource<CArchetype>(BaseArchetype);
	if (BaseArchetype) BaseArchetype->ValidateObject<CArchetype>();

	auto* pSession = static_cast<const Resources::CArchetypeLoader&>(Loader).GetSession();
	auto& ScriptState = pSession->GetScriptState();

	// TODO: check what is better, this or std::map<statname, desc> and runtime loop. Could help against duplicating stat lists here and in components.
	DEM::Meta::CMetadata<CArchetype>::ForEachMember([this, &ScriptState](const auto& Member)
	{
		if constexpr (std::is_same_v<DEM::Meta::TMemberValue<decltype(Member)>, std::unique_ptr<CNumericStatDefinition>>)
			LoadFormulaScript(Member.GetValueRef(*this).get(), ScriptState, Member.GetName());
	});

	// TODO:
	// maybe cache fallback pointers from base (need Ptr instead of unique_ptr then)
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
