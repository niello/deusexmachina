#include "ActionTpl.h"

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
ImplementRTTI(AI::CActionTpl, Core::CRefCounted);

void CActionTpl::Init(PParams Params)
{
	if (Params.isvalid())
	{
		Precedence = Params->Get(CStrID("Precedence"), 1);
		Cost = Params->Get(CStrID("Cost"), 1);
	}
}
//---------------------------------------------------------------------

} //namespace AI