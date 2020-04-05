#include "MemFactObstacle.h"

#include <AI/Perception/Stimulus.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CMemFactObstacle, 'MFOB', AI::CMemFact);

CMemFactObstacle::CMemFactObstacle() = default;
CMemFactObstacle::~CMemFactObstacle() = default;

bool CMemFactObstacle::Match(const CMemFact& Pattern, Data::CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactObstacle& PatternCast = (const CMemFactObstacle&)Pattern;

	if (pSourceStimulus.IsValidPtr() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

}