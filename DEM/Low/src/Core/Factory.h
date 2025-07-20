#pragma once
#include <StdCfg.h>
#include <string>
#include <unordered_map>

// Allows RTTI-enabled object creation by type name or FourCC code

namespace DEM::Core
{
class CRTTI;
class CRTTIBaseClass;

class CFactory
{
protected:

	std::unordered_map<std::string, const CRTTI*> NameToRTTI;
	std::unordered_map<uint32_t, const CRTTI*>    FourCCToRTTI;

	CFactory() = default;
	CFactory(const CFactory&) = delete;
	CFactory(CFactory&&) = delete;
	CFactory& operator =(const CFactory&) = delete;
	CFactory& operator =(CFactory&&) = delete;

public:

	static CFactory& Instance();

	void			Register(const CRTTI& RTTI, const char* pClassName, uint32_t FourCC = 0);
	bool			IsNameRegistered(const char* pClassName) const;
	bool			IsFourCCRegistered(uint32_t ClassFourCC) const;
	const CRTTI*	GetRTTI(const char* pClassName) const;
	const CRTTI*	GetRTTI(uint32_t ClassFourCC) const;
	CRTTIBaseClass*	Create(const char* pClassName) const;
	CRTTIBaseClass*	Create(uint32_t ClassFourCC) const;

	template<class T> T* Create(const char* pClassName) const
	{
		const CRTTI* pRTTI = GetRTTI(pClassName);

		// FIXME: use Verify instead of duplicating condition!
		n_assert2_dbg(pRTTI && pRTTI->IsDerivedFrom(T::RTTI), "No factory type or not a subclass: {}"_format(pClassName));
		return (pRTTI && pRTTI->IsDerivedFrom(T::RTTI)) ? static_cast<T*>(pRTTI->CreateInstance()) : nullptr;
	}

	template<class T> T* Create(uint32_t ClassFourCC) const
	{
		const CRTTI* pRTTI = GetRTTI(ClassFourCC);

		// FIXME: use Verify instead of duplicating condition!
		n_assert2_dbg(pRTTI && pRTTI->IsDerivedFrom(T::RTTI), "No factory type or not a subclass");
		return (pRTTI && pRTTI->IsDerivedFrom(T::RTTI)) ? static_cast<T*>(pRTTI->CreateInstance()) : nullptr;
	}
};

}
