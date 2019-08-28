#pragma once
#include <Core/RTTI.h>

// Base class for all RTTI-capable classes. Implements RTTI root and all necessary methods for type checking.

namespace Core
{

class CRTTIBaseClass
{
	__DeclareClassNoFactory;

public:

	//virtual ~CRTTIBaseClass() = default;

	bool			IsInstanceOf(const CRTTI& RTTI) const { return GetRTTI() == &RTTI; }
	bool			IsInstanceOf(const char* pName) const { return GetRTTI()->GetName() == pName; }
	bool			IsInstanceOf(Data::CFourCC FourCC) const { return GetRTTI()->GetFourCC() == FourCC; }
	template<class T>
	bool			IsA() const { return IsA(T::RTTI); }
	bool			IsA(const CRTTI& RTTI) const { return GetRTTI()->IsDerivedFrom(RTTI); }
	bool			IsA(const char* pName) const { return GetRTTI()->IsDerivedFrom(pName); }
	bool			IsA(Data::CFourCC FourCC) const { return GetRTTI()->IsDerivedFrom(FourCC); }
	template<class T>
	T*				As() { return IsA(T::RTTI) ? static_cast<T*>(this) : nullptr; }
	template<class T>
	const T*		As() const { return IsA(T::RTTI) ? static_cast<T*>(this) : nullptr; }
	const CString&	GetClassName() const { return GetRTTI()->GetName(); }
	Data::CFourCC	GetClassFourCC() const { return GetRTTI()->GetFourCC(); }
};

}
