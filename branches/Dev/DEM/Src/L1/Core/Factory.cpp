#include "Factory.h"

#include <Core/RTTI.h>

//???RTTI class name CStrID, CSimpleString or CString?

namespace Core
{

CFactory* CFactory::Instance()
{
	static CFactory Singleton;
	return &Singleton;
}
//---------------------------------------------------------------------

void CFactory::Register(const CRTTI& RTTI, const CString& Name, Data::CFourCC FourCC)
{
	n_assert2(!IsNameRegistered(Name), Name.CStr());
	if (FourCC != 0) n_assert2(!IsFourCCRegistered(FourCC), FourCC.ToString());
	NameToRTTI.Add(Name, &RTTI);
	if (FourCC != 0) FourCCToRTTI.Add(FourCC, &RTTI);
}
//---------------------------------------------------------------------

CRefCounted* CFactory::Create(const CString& ClassName, void* pParam) const
{
	n_assert2_dbg(IsNameRegistered(ClassName), ClassName.CStr());
	const CRTTI* pRTTI = GetRTTI(ClassName);
	n_assert_dbg(pRTTI);
	return pRTTI->CreateInstance(pParam);
}
//---------------------------------------------------------------------

CRefCounted* CFactory::Create(Data::CFourCC ClassFourCC, void* pParam) const
{
	n_assert2_dbg(IsFourCCRegistered(ClassFourCC), ClassFourCC.ToString());
	const CRTTI* pRTTI = GetRTTI(ClassFourCC);
	n_assert_dbg(pRTTI);
	return pRTTI->CreateInstance(pParam);
}
//---------------------------------------------------------------------

}