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

struct CAccessedStats
{
	std::vector<CNumericStat*> NumericStats;
	std::vector<CBoolStat*>    BoolStats;
};

class CCharacterSheet : public Data::CRefCounted
{
public:

	using TStat = std::variant<std::monostate, CNumericStat*, CBoolStat*/*, float*, bool**/>;

protected:

	std::map<std::string, TStat, std::less<>> _Stats; // TODO: unordered map with transparent comparator?
	mutable std::vector<CAccessedStats>       _StatTrackingStack; // Stats can be evaluated recursively, need to gather dependencies at each level

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
				_Stats.emplace(Member.GetName(), pStat);
				if constexpr (std::is_same_v<TMember, CNumericStat>)
					if (pStat->GetDesc() && pStat->GetDesc()->Formula) //???or set to all stats? archetype can change later?!
						pStat->SetSheet(this);
			}
		});
	}

public:

	static void RegisterScriptAPI(sol::state& State);

	CCharacterSheet(Game::CGameWorld& World, Game::HEntity EntityID);

	TStat          FindStat(std::string_view Name) const;

	void           BeginStatAccessTracking();
	CAccessedStats EndStatAccessTracking();
};

}
