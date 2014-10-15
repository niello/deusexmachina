#include "ActionTplWander.h"

#include <AI/Actions/ActionWander.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CActionTplWander, 'ATWN', AI::CActionTpl);

void CActionTplWander::Init(Data::PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_Action, CStrID("Wander"));
}
//---------------------------------------------------------------------

bool CActionTplWander::ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal)
{
	return !WSGoal.IsPropSet(WSP_UsingSmartObj) || WSGoal.GetProp(WSP_UsingSmartObj) == CStrID::Empty;
}
//---------------------------------------------------------------------

PAction CActionTplWander::CreateInstance(const CWorldState& Context) const
{
	return n_new(CActionWander);
}
//---------------------------------------------------------------------

} //namespace AI