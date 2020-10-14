#pragma once
#include <Core/RTTI.h>

// Base class for all RTTI-capable classes. Implements RTTI root and all necessary methods for type checking.

namespace Core
{

class CRTTIBaseClass
{
public:

	inline static const ::Core::CRTTI RTTI = ::Core::CRTTI("Core::CRTTIBaseClass", 0, nullptr, nullptr, 0); \

	virtual ~CRTTIBaseClass() = default;

	virtual const CRTTI* GetRTTI() const { return &RTTI; }

	bool               IsExactly(const CRTTI& RTTI) const { return GetRTTI() == &RTTI; }
	bool               IsExactly(const char* pName) const { return GetRTTI()->GetName() == pName; }
	bool               IsExactly(Data::CFourCC FourCC) const { return GetRTTI()->GetFourCC() == FourCC; }
	template<class T>
	bool               IsA() const { return IsA(T::RTTI); }
	bool               IsA(const CRTTI& RTTI) const { return GetRTTI()->IsDerivedFrom(RTTI); }
	bool               IsA(const char* pName) const { return GetRTTI()->IsDerivedFrom(pName); }
	bool               IsA(Data::CFourCC FourCC) const { return GetRTTI()->IsDerivedFrom(FourCC); }
	template<class T>
	T*                 As() { return IsA(T::RTTI) ? static_cast<T*>(this) : nullptr; }
	template<class T>
	const T*           As() const { return IsA(T::RTTI) ? static_cast<T*>(this) : nullptr; }
	const std::string& GetClassName() const { return GetRTTI()->GetName(); }
	Data::CFourCC      GetClassFourCC() const { return GetRTTI()->GetFourCC(); }
};

}
