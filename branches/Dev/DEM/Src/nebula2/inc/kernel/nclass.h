#ifndef N_CLASS_H
#define N_CLASS_H
//------------------------------------------------------------------------------
/**
    @class nClass
    @ingroup NebulaObjectSystem

    Nebula metaclass. nRoot derived objects are not created directly
    in C++, but by nClass objects. nClass objects wrap dynamic demand-loading
    of classes, and do other house holding stuff.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "util/nkeyarray.h"
#include "util/nhashnode.h"

class nRoot;
class nKernelServer;
class nHashList;
class nClass: public nHashNode
{
private:

	nClass*	superClass;
	nString	properName;
	int		refCount;
	int		instanceSize;

	bool	(*n_init_ptr)(nClass*, nKernelServer*);	// pointer to class init function
	void*	(*n_new_ptr)();							// pointer to object construction function

public:

	nClass(const char* name, bool (*initFunc)(nClass*, nKernelServer*), void* (*newFunc)());
	~nClass();

	nRoot*		NewObject();

	void		AddSubClass(nClass* cl) { n_assert(!cl->superClass); AddRef(); cl->superClass = this; }
	void		RemSubClass(nClass* cl) { n_assert(cl->superClass == this); Release(); cl->superClass = NULL; }

	int			AddRef() { return ++refCount; }
	int			Release() { n_assert(refCount > 0); return --refCount; }
	int			GetRefCount() const { return refCount; }

	void		SetProperName(const char* name) { properName = name; }
	const char*	GetProperName() const { return properName.Get(); }
	nClass*		GetSuperClass() const { return superClass; }
	bool		IsA(const char* className) const;
};

#endif
