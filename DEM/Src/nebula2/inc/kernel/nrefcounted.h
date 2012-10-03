#ifndef N_REFCOUNTED_H
#define N_REFCOUNTED_H
//------------------------------------------------------------------------------
/**
    @class nRefCounted
    @ingroup Kernel

    A simplest possible refcounted super class, if you need refcounting but
    don't want the nRoot overhead.

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/ntypes.h"

class nRefCounted
{
protected:

	int RefCount;

	virtual ~nRefCounted() { n_assert(!RefCount); }

public:

	nRefCounted(): RefCount(0) {}

	void	AddRef() { ++RefCount; }
	int		Release();
};

inline int nRefCounted::Release()
{
	n_assert(RefCount >= 0);
	if (RefCount) return --RefCount;
	n_delete(this);
	return 0;
}
//---------------------------------------------------------------------

#endif
