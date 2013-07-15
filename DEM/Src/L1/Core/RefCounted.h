#pragma once
#ifndef __DEM_L1_REFCOUNTED_H__
#define __DEM_L1_REFCOUNTED_H__

#include <Core/RTTI.h>
#include <Core/Factory.h>
#ifdef _DEBUG
#include "Data/List.h"
#endif

// Class with simple refcounting mechanism, and security check at application shutdown.

namespace Core
{
#ifdef _DEBUG
	typedef Data::CList<CRefCounted*> CRefCountedList;
#endif

class CRefCounted
{
	__DeclareClassNoFactory;

private:

	int RefCount; // volatile

#ifdef _DEBUG
	static CRefCountedList		List;
	CRefCountedList::CIterator	ListIt;
#endif

protected:

	virtual ~CRefCounted() = 0;

public:

	CRefCounted();

#ifdef _DEBUG
	static void DumpLeaks();
#endif

	void			AddRef() { ++RefCount; } //!!!interlocked for threading!
	void			Release() { n_assert(RefCount > 0); if (--RefCount == 0) n_delete(this); } //!!!interlocked for threading!
	int				GetRefCount() const { return RefCount; }
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

inline CRefCounted::CRefCounted(): RefCount(0)
{
#ifdef _DEBUG
	ListIt = List.AddBack(this);
#endif
}
//---------------------------------------------------------------------

}

#endif
