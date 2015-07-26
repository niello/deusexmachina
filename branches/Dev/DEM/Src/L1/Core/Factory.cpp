#include "Factory.h"

#include <Core/RTTI.h>

namespace Core
{

CFactory* CFactory::Instance()
{
	static CFactory Singleton;
	return &Singleton;
}
//---------------------------------------------------------------------

void CFactory::Register(const CRTTI& RTTI, const char* pClassName, Data::CFourCC FourCC)
{
	n_assert2(!IsNameRegistered(pClassName), pClassName);
	if (FourCC != 0) n_assert2(!IsFourCCRegistered(FourCC), FourCC.ToString());
	NameToRTTI.Add(CString(pClassName), &RTTI);
	if (FourCC != 0) FourCCToRTTI.Add(FourCC, &RTTI);
}
//---------------------------------------------------------------------

CRTTIBaseClass* CFactory::Create(const char* pClassName, void* pParam) const
{
	n_assert2_dbg(IsNameRegistered(pClassName), pClassName);
	const CRTTI* pRTTI = GetRTTI(pClassName);
	n_assert_dbg(pRTTI);
	return pRTTI->CreateClassInstance(pParam);
}
//---------------------------------------------------------------------

CRTTIBaseClass* CFactory::Create(Data::CFourCC ClassFourCC, void* pParam) const
{
	n_assert2_dbg(IsFourCCRegistered(ClassFourCC), ClassFourCC.ToString());
	const CRTTI* pRTTI = GetRTTI(ClassFourCC);
	n_assert_dbg(pRTTI);
	return pRTTI->CreateClassInstance(pParam);
}
//---------------------------------------------------------------------

}