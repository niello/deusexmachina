#pragma once
#ifndef __DEM_L1_CORE_FACTORY_H__
#define __DEM_L1_CORE_FACTORY_H__

// Allows object creation by type name or FourCC code

#include <Core/Ptr.h>
#include <util/HashTable.h>
#include <util/ndictionary.h>
#include <util/nstring.h>

namespace Core
{
class CRefCounted;
class CRTTI;

#define Factory Core::CFactory::Instance()

class CFactory
{
protected:

	CHashTable<nString, const CRTTI*>		NameToRTTI;
	nDictionary<nFourCC, const CRTTI*>		FourCCToRTTI; //???hash table too?

	CFactory(): NameToRTTI(512) {}

public:

	static CFactory* Instance();

	void			Register(const CRTTI& RTTI, const nString& Name, nFourCC FourCC = 0);
	bool			IsRegistered(const nString& ClassName) const { return NameToRTTI.Contains(ClassName); }
	bool			IsRegistered(nFourCC ClassFourCC) const { return FourCCToRTTI.Contains(ClassFourCC); }
	const CRTTI*	GetRTTI(const nString& ClassName) const { return NameToRTTI[ClassName]; }
	const CRTTI*	GetRTTI(nFourCC ClassFourCC) const { return FourCCToRTTI[ClassFourCC]; }
	CRefCounted*	Create(const nString& ClassName, void* pParam = NULL) const;
	CRefCounted*	Create(nFourCC ClassFourCC, void* pParam = NULL) const;
};

}

#endif
