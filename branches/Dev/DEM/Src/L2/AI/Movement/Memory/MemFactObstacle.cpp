#include "MemFactObstacle.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
__ImplementClassNoFactory(AI::CMemFactObstacle, AI::CMemFact);
__ImplementClass(AI::CMemFactObstacle);

bool CMemFactObstacle::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactObstacle& PatternCast = (const CMemFactObstacle&)Pattern;

	if (pSourceStimulus.IsValid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace AI