#include "MemFactObstacle.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
ImplementRTTI(AI::CMemFactObstacle, AI::CMemFact);
ImplementFactory(AI::CMemFactObstacle);

bool CMemFactObstacle::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactObstacle& PatternCast = (const CMemFactObstacle&)Pattern;

	if (pSourceStimulus.isvalid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace AI