#pragma once
#include <Data/StringID.h>
#include <sol/sol.hpp>
#include <vector>

// Smart object asset describes a set of states, transitions between them,
// and interactions available over the object under different conditions

namespace DEM::Game
{

class CSmartObject //???is resource?
{
protected:

	//transitions inside states? no custom logic = scripted OnEnter etc always?
	//!!!instance can cache OnEnter etc functions! or not instance, but class?
	struct CStateRecord
	{
		//state logic object
		//???optional state timeline(resource?)?
		//map or vector of transitions
	};

	std::vector<std::pair<CStrID, CStateRecord>>  States; //!!!sort by ID for fast search!
	std::vector<std::pair<CStrID, sol::function>> Interactions; // Interaction ID -> optional condition

public:

	const auto& GetInteractions() const { return Interactions; }
};

}
