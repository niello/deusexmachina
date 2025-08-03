#pragma once
#include <Character/ParameterModifier.h>
#include <Events/Signal.h>

// A character boolean stat value that can be temporarily altered by modifiers

namespace DEM::RPG
{

struct CBoolStatDefinition
{
	bool DefaultValue = false; // 'true' adds an innate enabler for this stat
	// no formula, can't come up with an idea of its usage now
	// inverted name for scripts and facade value getter - for cases like IsNotHexed -> IsHexed
};

class CBoolStat
{
protected:

	// Usually a status effect instance ID
	//!!!need special ID for innate effects from an archetype! zero?
	std::set<U32> _Enablers;
	std::set<U32> _Blockers;
	std::set<U32> _Immunity;

public:

	void AddEnabler(U32 SourceID) { _Enablers.insert(SourceID); }
	void AddBlocker(U32 SourceID) { _Blockers.insert(SourceID); }
	void AddImmunity(U32 SourceID) { _Immunity.insert(SourceID); }

	void Remove(U32 SourceID)
	{
		_Enablers.erase(SourceID);
		_Blockers.erase(SourceID);
		_Immunity.erase(SourceID);
	}

	bool Get() const { return !_Enablers.empty() && (_Blockers.empty() || !_Immunity.empty()); }
};

}
