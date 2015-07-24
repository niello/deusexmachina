#pragma once
#ifndef __DEM_L1_CORE_FACTORY_H__
#define __DEM_L1_CORE_FACTORY_H__

// Allows object creation by type name or FourCC code

#include <Data/Ptr.h>
#include <Data/FourCC.h>
#include <Data/HashTable.h>
#include <Data/Dictionary.h>
#include <Data/String.h>

namespace Core
{
class CObject;
class CRTTI;

#define Factory Core::CFactory::Instance()

class CFactory
{
protected:

	CHashTable<CString, const CRTTI*>	NameToRTTI;
	CDict<Data::CFourCC, const CRTTI*>	FourCCToRTTI; //???hash table too?

	CFactory(): NameToRTTI(512) {}

public:

	static CFactory* Instance();

	void			Register(const CRTTI& RTTI, const char* pClassName, Data::CFourCC FourCC = 0);
	bool			IsNameRegistered(const char* pClassName) const { return NameToRTTI.Contains(CString(pClassName)); }
	bool			IsFourCCRegistered(Data::CFourCC ClassFourCC) const { return FourCCToRTTI.Contains(ClassFourCC); }
	const CRTTI*	GetRTTI(const char* pClassName) const { return NameToRTTI[CString(pClassName)]; }
	const CRTTI*	GetRTTI(Data::CFourCC ClassFourCC) const { return FourCCToRTTI[ClassFourCC]; }
	CObject*		Create(const char* pClassName, void* pParam = NULL) const;
	CObject*		Create(Data::CFourCC ClassFourCC, void* pParam = NULL) const;
};

}

#endif
