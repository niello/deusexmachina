#include "ActionTpl.h"

#ifdef __WIN32__
	#ifdef GetProp
		#undef GetProp
		#undef SetProp
	#endif
#endif

namespace AI
{
__ImplementClassNoFactory(AI::CActionTpl, Core::CObject);

void CActionTpl::Init(Data::PParams Params)
{
	if (Params.IsValid())
	{
		Precedence = Params->Get(CStrID("Precedence"), 1);
		Cost = Params->Get(CStrID("Cost"), 1);
	}
}
//---------------------------------------------------------------------

}