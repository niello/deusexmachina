#include "MemFactSmartObj.h"

#include <AI/Perception/Stimulus.h>
#include <Core/Factory.h>

namespace AI
{
FACTORY_CLASS_IMPL(AI::CMemFactSmartObj, 'MFSO', AI::CMemFact);

bool CMemFactSmartObj::Match(const CMemFact& Pattern, Data::CFlags FieldMask) const
{
	if (!CMemFact::Match(Pattern, FieldMask)) FAIL;

	const CMemFactSmartObj& PatternCast = (const CMemFactSmartObj&)Pattern;

	if (pSourceStimulus.IsValidPtr() && pSourceStimulus != PatternCast.pSourceStimulus) FAIL;

	OK;
}
//---------------------------------------------------------------------

}