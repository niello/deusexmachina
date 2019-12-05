#include "MemFact.h"

namespace AI
{
RTTI_CLASS_IMPL(AI::CMemFact, Core::CObject);

bool CMemFact::Match(const CMemFact& Pattern, Data::CFlags FieldMask) const
{
	// Nothing to check for now
	n_assert(Pattern.GetKey() == GetKey()); // If this assertion fails, remove it and check type matching here
	OK;
}
//---------------------------------------------------------------------

}