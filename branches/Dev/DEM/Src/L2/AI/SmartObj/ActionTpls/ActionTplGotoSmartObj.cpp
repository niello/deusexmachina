#include "ActionTplGotoSmartObj.h"

#include <AI/SmartObj/Actions/ActionGotoSmartObj.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClass(AI::CActionTplGotoSmartObj, 'ATGS', AI::CActionTpl);

void CActionTplGotoSmartObj::Init(PParams Params)
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
	PActionGotoSmartObj Act = n_new(CActionGotoSmartObj);
	Act->Init(Context.GetProp(WSP_AtEntityPos), Context.GetProp(WSP_Action));
	return Act.GetUnsafe();
}
//---------------------------------------------------------------------

} //namespace AI