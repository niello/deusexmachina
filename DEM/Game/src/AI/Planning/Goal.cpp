#include "Goal.h"

namespace AI
{

void CGoal::Init(Data::PParams Params)
{
	if (Params.IsValidPtr())
		PersonalityFactor = Params->Get(CStrID("PersonalityFactor"), 1.f);
}
//---------------------------------------------------------------------

}
