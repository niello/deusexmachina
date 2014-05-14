#include "Goal.h"

namespace AI
{
__ImplementClassNoFactory(AI::CGoal, Core::CObject);

void CGoal::Init(Data::PParams Params)
{
	if (Params.IsValid())
		PersonalityFactor = Params->Get(CStrID("PersonalityFactor"), 1.f);
}
//---------------------------------------------------------------------

}