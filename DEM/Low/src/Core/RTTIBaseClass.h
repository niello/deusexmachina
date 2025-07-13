#pragma once
#include <Core/RTTI.h>

// Base class for all RTTI-capable classes. Implements RTTI root and all necessary methods for type checking.

namespace DEM::Core
{

class CRTTIBaseClass
{
public:

	inline static const CRTTI RTTI = CRTTI("DEM::Core::CRTTIBaseClass", 0, nullptr, nullptr, nullptr, 0, 0);

	virtual ~CRTTIBaseClass() = default;

	virtual const CRTTI* GetRTTI() const { return &RTTI; }

	template<typename T>
	bool               IsExactly() const { return IsExactly(T::RTTI); }
	template<typename... T>
	bool               IsAnyOfExactly() const { return (IsExactly<T>() || ...); }
	bool               IsExactly(const CRTTI& RTTI) const { return GetRTTI() == &RTTI; }
	bool               IsExactly(const char* pName) const { return GetRTTI()->GetName() == pName; }
	bool               IsExactly(Data::CFourCC FourCC) const { return GetRTTI()->GetFourCC() == FourCC; }
	template<typename T>
	bool               IsA() const { return IsA(T::RTTI); }
	template<typename... T>
	bool               IsAnyOf() const { return (IsA<T>() || ...); }
	bool               IsA(const CRTTI& RTTI) const { return GetRTTI()->IsDerivedFrom(RTTI); }
	bool               IsA(const char* pName) const { return GetRTTI()->IsDerivedFrom(pName); }
	bool               IsA(Data::CFourCC FourCC) const { return GetRTTI()->IsDerivedFrom(FourCC); }
	template<typename T>
	T*                 As() { return IsA(T::RTTI) ? static_cast<T*>(this) : nullptr; }
	template<typename T>
	const T*           As() const { return IsA(T::RTTI) ? static_cast<T*>(this) : nullptr; }
	const std::string& GetClassName() const { return GetRTTI()->GetName(); }
	Data::CFourCC      GetClassFourCC() const { return GetRTTI()->GetFourCC(); }
};

}
