#include "MemFactSmartObj.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
__ImplementClass(AI::CMemFactSmartObj, 'MFSO', AI::CMemFact);

bool CMemFactSmartObj::Match(const CMemFact& Pattern, Data::CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactSmartObj& PatternCast = (const CMemFactSmartObj&)Pattern;

	if (pSourceStimulus.IsValid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

}