#include "Goal.h"

namespace AI
{
RTTI_CLASS_IMPL(AI::CGoal, Core::CObject);

void CGoal::Init(Data::PParams Params)
{
	if (Params.IsValidPtr())
		PersonalityFactor = Params->Get(CStrID("PersonalityFactor"), 1.f);
}
//---------------------------------------------------------------------

}