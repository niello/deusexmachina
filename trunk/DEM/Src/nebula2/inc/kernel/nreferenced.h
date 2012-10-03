#ifndef N_REFERENCED_H
#define N_REFERENCED_H
//------------------------------------------------------------------------------
/**
    @class nReferenced
    @ingroup Kernel

    Provides simple reference counting as well as tracking references to self.
    Never destroy nReferenced objects through delete.
*/

#include "util/nlist.h"

template<class TYPE> class nRef;

class nReferenced
{
protected:

	nList RefList;
	int RefCount;

	virtual ~nReferenced() { n_assert(!RefCount); InvalidateAllRefs(); }

	void			InvalidateAllRefs();

public:

	nReferenced(): RefCount(0) {}

	int				AddRef() { return ++RefCount; }
	virtual bool	Release(); /// release object (USE INSTEAD OF DESTRUCTOR!)

	void			AddObjectRef(nRef<nReferenced>* r) { RefList.AddTail((nNode*)r); }
	void			RemObjectRef(nRef<nReferenced>* r) { ((nNode*)r)->Remove(); }

	nList*			GetRefs() { return &RefList; }
	int				GetRefCount() const { return RefCount; }
};

inline bool nReferenced::Release()
{
	n_assert(RefCount > 0);
	if (--RefCount) return false;
	n_delete(this);
	return true;
}
//---------------------------------------------------------------------

#endif // N_REFERENCED_H
