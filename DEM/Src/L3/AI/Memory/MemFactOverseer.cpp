#include "MemFactOverseer.h"

#include <AI/Perception/Stimulus.h>

namespace AI
{
__ImplementClassNoFactory(AI::CMemFactOverseer, AI::CMemFact);
__ImplementClass(AI::CMemFactOverseer);

bool CMemFactOverseer::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactOverseer& PatternCast = (const CMemFactOverseer&)Pattern;

	if (pSourceStimulus.IsValid() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

} //namespace AI