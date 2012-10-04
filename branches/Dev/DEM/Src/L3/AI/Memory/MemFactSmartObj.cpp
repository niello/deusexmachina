#include "MemFactSmartObj.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
ImplementRTTI(AI::CMemFactSmartObj, AI::CMemFact);
ImplementFactory(AI::CMemFactSmartObj);

bool CMemFactSmartObj::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactSmartObj& PatternCast = (const CMemFactSmartObj&)Pattern;

	if (pSourceStimulus.isvalid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace AI