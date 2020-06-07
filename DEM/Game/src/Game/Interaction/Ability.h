#pragma once
#include <vector>

// 

namespace DEM::Game
{
class CAction;

class CAbility
{
protected:

	std::vector<CAction> Actions; //???PAction?

public:

	CAction* ChooseAction() const;

	// list of actions available, including smart object action proxy
	//???need something specific for switchable abilities (on-off) or is controlled from outside?

	// rendering info? icon, cursor
};

}
