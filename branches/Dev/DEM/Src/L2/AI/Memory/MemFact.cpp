#include "MemFact.h"

namespace AI
{
ImplementRTTI(AI::CMemFact, Core::CRefCounted);

bool CMemFact::Match(const CMemFact& Pattern, CFlags FieldMask) const
{
	// Nothing to check for now
	n_assert(Pattern.GetKey() == GetKey()); // If this assertion fails, remove it and check type matching here
	OK;
}
//---------------------------------------------------------------------

}