#include "ConditionRegistry.h"
#include <Scripting/Flow/Condition.h> // also for PCondition destruction

namespace DEM::Flow
{

CConditionRegistry::CConditionRegistry(Game::CGameSession& Owner)
	: _Session(Owner)
{
	// Register standard condition implementations
	RegisterCondition<CFalseCondition>(CFalseCondition::Type);
	RegisterCondition<CAndCondition>(CAndCondition::Type);
	RegisterCondition<COrCondition>(COrCondition::Type);
	RegisterCondition<CNotCondition>(CNotCondition::Type);
	RegisterCondition<CVarCmpConstCondition>(CVarCmpConstCondition::Type);
	RegisterCondition<CVarCmpVarCondition>(CVarCmpVarCondition::Type);
	RegisterCondition<CLuaStringCondition>(CLuaStringCondition::Type);
}
//---------------------------------------------------------------------

CConditionRegistry::~CConditionRegistry() = default;
//---------------------------------------------------------------------

}
