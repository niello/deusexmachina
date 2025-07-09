#include "Factory.h"
#include <Core/RTTI.h>

namespace DEM::Core
{

CFactory& CFactory::Instance()
{
	static CFactory Singleton;
	return Singleton;
}
//---------------------------------------------------------------------

void CFactory::Register(const CRTTI& RTTI, const char* pClassName, uint32_t FourCC)
{
	n_assert2(!IsNameRegistered(pClassName), pClassName);
	if (FourCC != 0) n_assert2(!IsFourCCRegistered(FourCC), pClassName);
	NameToRTTI.emplace(std::string(pClassName), &RTTI);
	if (FourCC != 0) FourCCToRTTI.emplace(FourCC, &RTTI);
}
//---------------------------------------------------------------------

bool CFactory::IsNameRegistered(const char* pClassName) const
{
	return pClassName && NameToRTTI.find(std::string(pClassName)) != NameToRTTI.cend();
}
//---------------------------------------------------------------------

bool CFactory::IsFourCCRegistered(uint32_t ClassFourCC) const
{
	return FourCCToRTTI.find(ClassFourCC) != FourCCToRTTI.cend();
}
//---------------------------------------------------------------------

const CRTTI* CFactory::GetRTTI(const char* pClassName) const
{
	if (!pClassName) return nullptr;

	auto It = NameToRTTI.find(std::string(pClassName));
	return (It == NameToRTTI.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

const CRTTI* CFactory::GetRTTI(uint32_t ClassFourCC) const
{
	auto It = FourCCToRTTI.find(ClassFourCC);
	return (It == FourCCToRTTI.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

CRTTIBaseClass* CFactory::Create(const char* pClassName) const
{
	const CRTTI* pRTTI = GetRTTI(pClassName);
	n_assert_dbg(pRTTI);
	return pRTTI ? pRTTI->CreateInstance() : nullptr;
}
//---------------------------------------------------------------------

CRTTIBaseClass* CFactory::Create(uint32_t ClassFourCC) const
{
	const CRTTI* pRTTI = GetRTTI(ClassFourCC);
	n_assert_dbg(pRTTI);
	return pRTTI ? pRTTI->CreateInstance() : nullptr;
}
//---------------------------------------------------------------------

}
