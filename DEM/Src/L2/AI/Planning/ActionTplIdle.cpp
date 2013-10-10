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
__ImplementClass(AI::CActionTplIdle, 'ATID', AI::CActionTpl);

void CActionTplIdle::Init(Data::PParams Params)
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

}