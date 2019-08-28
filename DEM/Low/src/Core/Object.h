#pragma once
#include <Data/RefCounted.h>
#include <Core/RTTIBaseClass.h>
#ifdef _DEBUG
#include <Data/List.h>
#endif

// Base class for engine objects. Provides refcounting, smart pointer support,
// fast custom RTTI and security check at application shutdown.

namespace Core
{
typedef Ptr<class CObject> PObject;

class CObject: public Data::CRefCounted, public CRTTIBaseClass
{
	__DeclareClassNoFactory;

#ifdef _DEBUG
private:

	typedef Data::CList<CObject*> CObjList;

	static CObjList		List;
	CObjList::CIterator	ListIt;

protected:

	virtual ~CObject() { n_assert(ListIt); List.Remove(ListIt); }

public:

	CObject() { ListIt = List.AddBack(this); }

	static void DumpLeaks();
#endif
};

}
