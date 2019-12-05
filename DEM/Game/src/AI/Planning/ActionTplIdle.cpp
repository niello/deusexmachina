#include "ActionTplIdle.h"

#include <AI/Behaviour/ActionIdle.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CActionTplIdle, 'ATID', AI::CActionTpl);

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