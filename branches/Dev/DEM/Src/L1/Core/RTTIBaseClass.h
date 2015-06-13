#pragma once
#ifndef __DEM_L1_RTTI_BASE_CLASS_H__
#define __DEM_L1_RTTI_BASE_CLASS_H__

#include <Core/RTTI.h>

// Base class for all RTTI-capable classes. Implements RTTI root and all necessary methods for type checking.

namespace Core
{

class CRTTIBaseClass
{
	__DeclareClassNoFactory;

public:

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

}

#endif
