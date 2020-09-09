#pragma once
#include <Resources/ResourceObject.h>
#include <Data/StringID.h>
#include <sol/sol.hpp>
#include <vector>

// Smart object asset describes a set of states, transitions between them,
// and interactions available over the object under different conditions

namespace DEM::Game
{
using PSmartObject = Ptr<class CSmartObject>;

class CSmartObject : public Resources::CResourceObject
{
	RTTI_CLASS_DECL;

protected:

	//transitions inside states? no custom logic = scripted OnEnter etc always?
	//!!!instance can cache OnEnter etc functions! or not instance, but class?
	struct CStateRecord
	{
		//state logic object (optional)
		//timeline asset (optional) + start, end, speed, [loop count - or always infinite for states]
		//map or vector of transitions:
		// - target state ID
		// - timeline asset (optional) + start, end, speed, loop count 
	};

	//state callback Lua functions cached (OnStateEnter etc)

	std::vector<std::pair<CStrID, CStateRecord>>  States; //!!!sort by ID for fast search!
	std::vector<std::pair<CStrID, sol::function>> Interactions; // Interaction ID -> optional condition

public:

	virtual bool IsResourceValid() const override { return !States.empty(); }

	const auto& GetInteractions() const { return Interactions; }
};

}
