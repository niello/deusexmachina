#include "MemFactSmartObj.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
__ImplementClassNoFactory(AI::CMemFactSmartObj, AI::CMemFact);
__ImplementClass(AI::CMemFactSmartObj);

bool CMemFactSmartObj::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactSmartObj& PatternCast = (const CMemFactSmartObj&)Pattern;

	if (pSourceStimulus.IsValid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace AI