#pragma once
#ifndef __DEM_L1_OBJECT_H__
#define __DEM_L1_OBJECT_H__

#include <Data/RefCounted.h>
#include <Core/RTTI.h>
#ifdef _DEBUG
#include <Data/List.h>
#endif

// Base class for objects. Provides refcounting, smart pointer support,
// fast custom RTTI and security check at application shutdown.

namespace Core
{

class CObject: public CRefCounted //???make mix-in instead of refcounted child? rename to CRTTIClass, and CObject: public CRefCounted, CRTTIClass
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

	bool			IsInstanceOf(const CRTTI& RTTI) const { return GetRTTI() == &RTTI; }
	bool			IsInstanceOf(const CString& Name) const { return GetRTTI()->GetName() == Name; }
	bool			IsInstanceOf(Data::CFourCC FourCC) const { return GetRTTI()->GetFourCC() == FourCC; }
	template<class T>
	bool			IsA() const { return IsA(T::RTTI); }
	bool			IsA(const CRTTI& RTTI) const { return GetRTTI()->IsDerivedFrom(RTTI); }
	bool			IsA(const CString& Name) const { return GetRTTI()->IsDerivedFrom(Name); }
	bool			IsA(Data::CFourCC FourCC) const { return GetRTTI()->IsDerivedFrom(FourCC); }
	const CString&	GetClassName() const { return GetRTTI()->GetName(); }
	Data::CFourCC	GetClassFourCC() const { return GetRTTI()->GetFourCC(); }
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
