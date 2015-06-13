#pragma once
#ifndef __DEM_L1_OBJECT_H__
#define __DEM_L1_OBJECT_H__

#include <Data/RefCounted.h>
#include <Core/RTTIBaseClass.h>
#ifdef _DEBUG
#include <Data/List.h>
#endif

// Base class for engine objects. Provides refcounting, smart pointer support,
// fast custom RTTI and security check at application shutdown.

namespace Core
{

class CObject: public Data::CRefCounted, public CRTTIBaseClass
{
	__DeclareClassNoFactory;

private:

#ifdef _DEBUG
	typedef Data::CList<CObject*> CObjList;

	static CObjList		List;
	CObjList::CIterator	ListIt;
#endif

protected:

	// NB: the destructor of derived classes MUST be virtual!
	virtual ~CObject() = 0;

public:

	CObject();

#ifdef _DEBUG
	static void DumpLeaks();
#endif
};
//---------------------------------------------------------------------

inline CObject::CObject()
{
#ifdef _DEBUG
	ListIt = List.AddBack(this);
#endif
}
//---------------------------------------------------------------------

inline CObject::~CObject()
{
#ifdef _DEBUG
	n_assert(ListIt);
	List.Remove(ListIt);
	ListIt = NULL;
#endif
}
//---------------------------------------------------------------------

}

#endif
