//------------------------------------------------------------------------------
//  (c) 2004 Vadim Macagon
//  Refactored out of nRoot.
//------------------------------------------------------------------------------
#include "kernel/nreferenced.h"
#include "kernel/nref.h"

void nReferenced::InvalidateAllRefs()
{
	nRef<nReferenced>* r;
	while (r = (nRef<nReferenced>*)RefList.GetHead())
		r->invalidate();
}
//---------------------------------------------------------------------
