#ifndef N_REF_H
#define N_REF_H
//------------------------------------------------------------------------------
/**
    @class nRef
    @ingroup NebulaSmartPointers

    nRef implements safe pointers to nReferenced derived objects which will
    invalidate themselves when the target object goes away. Usage of nRef helps
	you avoid dangling pointers and also protects against dereferencing a null pointer.

    Operations:

    Assigning ptr to ref:
    ref=ptr OR ref.set(ptr)

    Invalidating:
    ref=0 OR ref.invalidate() OR ref.set(0)

    Checking if pointer is valid (non-null):
    ref.isvalid()



    - 06-May-04     floh    added more operator, so that nRef's can be
                            used more like normal C++ pointers

    (C) 1999 RadonLabs GmbH
*/

#include "kernel/nreferenced.h"

//------------------------------------------------------------------------------
template<class TYPE>
class nRef: nNode
{
protected:

	TYPE* pObj;

public:

	nRef(): pObj(NULL) {}
	nRef(TYPE* o): pObj(o) { n_assert(o); ((nReferenced*)pObj)->AddObjectRef((nRef<nReferenced>*)this); }
	nRef(const nRef& rhs): pObj(rhs.get_unsafe()) { if (pObj) ((nReferenced*)pObj)->AddObjectRef((nRef<nReferenced>*)this); }
	~nRef() { invalidate(); }

	bool	isvalid() const { return pObj != NULL; }
	void	invalidate();
	void	set(TYPE* obj);
	TYPE*	get() const { n_assert2(pObj, "Null pointer access through nRef!"); return pObj; }
	TYPE*	get_unsafe() const { return pObj; }

	nRef& operator =(TYPE* obj) { set(obj); return *this; }
	nRef& operator =(const nRef& rhs) { set(rhs.pObj); return *this; }

	bool operator ==(const nRef& rhs) { return pObj == rhs.pObj; }
	bool operator !=(const nRef<TYPE>& rhs) { return pObj != rhs.pObj; }
	bool operator ==(TYPE* obj) { return obj == pObj; }
	bool operator !=(TYPE* obj) { return obj != pObj; }

	TYPE* operator ->() const { return get(); }
	TYPE& operator *() const { return *get(); }
	operator TYPE*() const { return get(); }
};

template<class TYPE>
inline void nRef<TYPE>::invalidate()
{
	if (pObj)
	{
		((nReferenced*)pObj)->RemObjectRef((nRef<nReferenced>*)this);
		pObj = NULL;
	}
}
//---------------------------------------------------------------------

template<class TYPE>
inline void nRef<TYPE>::set(TYPE* obj)
{
	invalidate();
	pObj = obj;
	if (obj) ((nReferenced*)pObj)->AddObjectRef((nRef<nReferenced>*)this);
}
//---------------------------------------------------------------------

#endif
