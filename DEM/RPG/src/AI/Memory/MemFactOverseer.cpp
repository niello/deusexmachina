#include "MemFactOverseer.h"

#include <AI/Perception/Stimulus.h>
#include <Core/Factory.h>

namespace AI
{
__ImplementClass(AI::CMemFactOverseer, 'MFOV', AI::CMemFact);

bool CMemFactOverseer::Match(const CMemFact& Pattern, Data::CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactOverseer& PatternCast = (const CMemFactOverseer&)Pattern;

	if (pSourceStimulus.IsValidPtr() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

}