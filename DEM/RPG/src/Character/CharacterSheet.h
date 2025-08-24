#pragma once
#include <Game/ECS/Entity.h>
#include <variant>

// The sheet is an interface for all stats in one place, like in a tabletop RPG system.
// Different stats can live in different components, e.g. Strength is in "stats" and HP
// is in "destructible". The sheet simplifies implementing RPG rules and formulas,
// especially scripted. Marking accessed stats supported for dependency tracking.

namespace sol
{
	class state;
}

namespace DEM::Game
{
	class CGameWorld;
}

namespace DEM::RPG
{
class CNumericStat;
class CBoolStat;
using PCharacterSheet = Ptr<class CCharacterSheet>;

class CCharacterSheet : public Data::CRefCounted
{
public:

	using TStat = std::variant<std::monostate, CNumericStat*, CBoolStat*/*, float*, bool**/>;

protected:

	//!!!FIXME: instead of variant, can use own 2-type union!
	struct CRecord
	{
		TStat Stat;
		bool  Accessed = false;
	};

	std::map<std::string, CRecord, std::less<>> _Stats; // TODO: unordered map with transparent comparator?

	// TODO: listen component add/remove to fix stats, _if_ dynamic changing of stat components must be supported

	template<typename T>
	void CollectStatsFromComponent(T* pComponent)
	{
		DEM::Meta::CMetadata<T>::ForEachMember([this, pComponent](const auto& Member)
		{
			using TMember = DEM::Meta::TMemberValue<decltype(Member)>;
			if constexpr (std::is_same_v<TMember, CNumericStat> || std::is_same_v<TMember, CBoolStat>)
			{
				auto* pStat = Member.GetValuePtr(*pComponent);
				_Stats.emplace(Member.GetName(), CRecord{ pStat, false });
				if constexpr (std::is_same_v<TMember, CNumericStat>)
					if (pStat->GetDesc() && pStat->GetDesc()->Formula) //???or set to all stats? archetype can change later?!
						pStat->SetSheet(this);
			}
		});
	}

public:

	// TODO: iterate and clear accessed stats. Can pass a numeric stat to subscribe? Or should be outside?!!!

	static void RegisterScriptAPI(sol::state& State);

	CCharacterSheet(Game::CGameWorld& World, Game::HEntity EntityID);

	TStat FindStat(std::string_view Name) const;
	TStat FindStatAndRegisterAccess(std::string_view Name);
};

}
