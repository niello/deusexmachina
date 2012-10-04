#include "Goal.h"

namespace AI
{
ImplementRTTI(AI::CGoal, Core::CRefCounted);

void CGoal::Init(PParams Params)
{
	if (Params.isvalid())
		PersonalityFactor = Params->Get(CStrID("PersonalityFactor"), 1.f);
}
//---------------------------------------------------------------------

} //namespace AI