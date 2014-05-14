#pragma once
#ifndef __DEM_L1_REFCOUNTED_H__
#define __DEM_L1_REFCOUNTED_H__

#include <Data/Ptr.h>

// Class with simple refcounting mechanism.
// Paired with Ptr<> template class, which implements a smart pointer over this class.

//!!!can add check as in CRefCounted! can't move check here because there is no class name here!

class CRefCounted
{
private:

	int RefCount; // volatile

public:

	CRefCounted(): RefCount(0) {}
	virtual ~CRefCounted() { n_assert(!RefCount); }

	void			AddRef() { ++RefCount; } //!!!interlocked for threading!
	void			Release() { n_assert(RefCount > 0); if (--RefCount == 0) n_delete(this); } //!!!interlocked for threading!
	int				GetRefCount() const { return RefCount; }
};

#endif
