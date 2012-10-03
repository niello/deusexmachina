#ifndef N_DYNAUTOREF_H
#define N_DYNAUTOREF_H
//------------------------------------------------------------------------------
/**
    @class nDynAutoRef
    @ingroup NebulaSmartPointers

    nAutoRef with dynamically allocated name string.
*/
#include "kernel/nref.h"

//------------------------------------------------------------------------------
template<class TYPE>
class nDynAutoRef: public nRef<TYPE>
{
protected:

	const char* pName;

	TYPE* check();

public:

	nDynAutoRef(): pName(NULL) {}
	nDynAutoRef(const char* pTargetName): nRef<TYPE>(), pName(NULL) { set(pTargetName); }
	~nDynAutoRef() { if (pName) n_free((void*)pName); }

	nDynAutoRef& operator =(const nDynAutoRef& rhs) { set(rhs.getname()); return *this; }

	TYPE* get();
	const char* getname() const { return pName; }
	bool isvalid() { return !!check(); }
	void set(const char* pTargetName);

	void operator =(const char* name) { set(name); }
	void operator =(TYPE* obj) { invalidate(); nRef<TYPE>::set(obj); }
	TYPE* operator ->() { return get(); }
	TYPE& operator *() { return *get(); }
	operator TYPE*() { return get(); }
};

template<class TYPE>
inline void nDynAutoRef<TYPE>::set(const char* pTargetName)
{
	invalidate();
	if (pName) n_free((void*)pName);
	pName = pTargetName ? n_strdup(pTargetName) : NULL;
}
//---------------------------------------------------------------------

template<class TYPE>
inline TYPE* nDynAutoRef<TYPE>::check()
{
    if (!pObj)
    {
        if (!pName) return NULL;
        pObj = (TYPE*)nKernelServer::Instance()->Lookup(pName);
        if (pObj) ((nReferenced*)pObj)->AddObjectRef((nRef<nReferenced>*)this);
    }
    return pObj;
}
//---------------------------------------------------------------------

template<class TYPE>
inline TYPE* nDynAutoRef<TYPE>::get()
{
    if (!check()) n_error("nDynAutoRef: no target object '%s'!\n",  pName ? pName : "NOT INITIALIZED");
    return pObj;
}
//---------------------------------------------------------------------

#endif
