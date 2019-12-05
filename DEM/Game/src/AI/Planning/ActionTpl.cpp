#include "ActionTpl.h"

namespace AI
{
RTTI_CLASS_IMPL(AI::CActionTpl, Core::CObject);

void CActionTpl::Init(Data::PParams Params)
{
	if (Params.IsValidPtr())
	{
		Precedence = Params->Get(CStrID("Precedence"), 1);
		Cost = Params->Get(CStrID("Cost"), 1);
	}
}
//---------------------------------------------------------------------

}