#pragma once
#ifndef __DEM_L1_CORE_FACTORY_H__
#define __DEM_L1_CORE_FACTORY_H__

// Facilities for creating objects by type name to support load/save mechanism.

#include <Core/Ptr.h>
#include <util/HashMap.h>
#include <util/nstring.h>

namespace Core
{
class CRefCounted;
class CRTTI;

typedef CRefCounted* (*CFactoryFunction)();

#define CoreFct Core::CFactory::Instance()

class CFactory
{
protected:

	CHashMap<CFactoryFunction> ClassTable;

	CFactory(): ClassTable(NULL, 512) {}

public:

	static CFactory* Instance();

	void			Add(CFactoryFunction Function, const nString& ClassName);
	bool			Has(const nString& ClassName) const { return ClassTable.Contains(ClassName.Get()); }
	CRefCounted*	Create(const nString& ClassName) const;
	CRefCounted*	Create(const CRTTI& Class) const;

	int				GetNumClassNames() const { return ClassTable.Size(); }
};

}

#endif
