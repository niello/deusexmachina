#pragma once
#ifndef __DEM_L1_CORE_FACTORY_H__
#define __DEM_L1_CORE_FACTORY_H__

// Allows object creation by type name or FourCC code

#include <Core/Ptr.h>
#include <Data/FourCC.h>
#include <Data/HashTable.h>
#include <Data/Dictionary.h>
#include <Data/String.h>

namespace Core
{
class CRefCounted;
class CRTTI;

#define Factory Core::CFactory::Instance()

class CFactory
{
protected:

	CHashTable<CString, const CRTTI*>		NameToRTTI;
	CDict<Data::CFourCC, const CRTTI*>		FourCCToRTTI; //???hash table too?

	CFactory(): NameToRTTI(512) {}

public:

	static CFactory* Instance();

	void			Register(const CRTTI& RTTI, const CString& Name, Data::CFourCC FourCC = 0);
	bool			IsNameRegistered(const CString& ClassName) const { return NameToRTTI.Contains(ClassName); }
	bool			IsFourCCRegistered(Data::CFourCC ClassFourCC) const { return FourCCToRTTI.Contains(ClassFourCC); }
	const CRTTI*	GetRTTI(const CString& ClassName) const { return NameToRTTI[ClassName]; }
	const CRTTI*	GetRTTI(Data::CFourCC ClassFourCC) const { return FourCCToRTTI[ClassFourCC]; }
	CRefCounted*	Create(const CString& ClassName, void* pParam = NULL) const;
	CRefCounted*	Create(Data::CFourCC ClassFourCC, void* pParam = NULL) const;
};

}

#endif
