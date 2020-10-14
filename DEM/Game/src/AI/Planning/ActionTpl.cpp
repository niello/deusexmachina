#include "ActionTpl.h"

namespace AI
{

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
