#include "ActionTplIdle.h"

#include <AI/Behaviour/ActionIdle.h>

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClassNoFactory(AI::CActionTplIdle, AI::CActionTpl);
__ImplementClass(AI::CActionTplIdle);

void CActionTplIdle::Init(PParams Params)
{
	CActionTpl::Init(Params);
	WSEffects.SetProp(WSP_Action, CStrID("Idle"));
}
//---------------------------------------------------------------------

PAction CActionTplIdle::CreateInstance(const CWorldState& Context) const
{
	return n_new(CActionIdle);
}
//---------------------------------------------------------------------

} //namespace AI