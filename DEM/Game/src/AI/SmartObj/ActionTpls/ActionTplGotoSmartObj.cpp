#include "ActionTplGotoSmartObj.h"

//#include <AI/SmartObj/Actions/ActionGotoSmartObj.h>
#include <AI/Behaviour/Action.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CActionTplGotoSmartObj, 'ATGS', AI::CActionTpl);

void CActionTplGotoSmartObj::Init(Data::PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_AtEntityPos, WSP_AtEntityPos);
}
//---------------------------------------------------------------------

bool CActionTplGotoSmartObj::ValidateContextPreconditions(CActor* pActor, const CWorldState& WSGoal)
{
	//!!!Check dest pos & path to it!
	return WSGoal.IsPropSet(WSP_UsingSmartObj) && WSGoal.GetProp(WSP_UsingSmartObj) != CStrID::Empty;
}
//---------------------------------------------------------------------

PAction CActionTplGotoSmartObj::CreateInstance(const CWorldState& Context) const
{
	//PActionGotoSmartObj Act = n_new(CActionGotoSmartObj);
	//Act->Init(Context.GetProp(WSP_AtEntityPos), Context.GetProp(WSP_Action));
	//return Act.Get();
	return PAction();
}
//---------------------------------------------------------------------

}
