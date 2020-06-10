#pragma once
#include <vector>

// 

namespace DEM::Game
{
class CAction;

class CAbility
{
protected:

	std::vector<CAction> Actions; //???PAction? can be smart object action etc.

public:

	// can add factory methods to abilities and actions to create from CParams etc

	CAction* ChooseAction() const;

	//???need something specific for switchable abilities (on-off) or is controlled from outside?

	// rendering info? icon, cursor
};

}
