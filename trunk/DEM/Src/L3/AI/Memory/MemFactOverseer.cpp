#include "MemFactOverseer.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
ImplementRTTI(AI::CMemFactOverseer, AI::CMemFact);
ImplementFactory(AI::CMemFactOverseer);

bool CMemFactOverseer::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactOverseer& PatternCast = (const CMemFactOverseer&)Pattern;

	if (pSourceStimulus.isvalid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace AI