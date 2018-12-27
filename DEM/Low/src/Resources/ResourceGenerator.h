#pragma once
#ifndef __DEM_L1_RESOURCE_GENERATOR_H__
#define __DEM_L1_RESOURCE_GENERATOR_H__

#include <Core/Object.h>

// Resource generator incapsulates a procedural creation algorithm for a particular
// resource object type. It stores all settings necessary to create and initialize
// a resource object. Generator, being attached to a CResource, allows to recreate
// it automatically on resource lost. Derive from this class to implement different
// resource object generator groups and algorithms.

namespace Resources
{
typedef Ptr<class CResourceObject> PResourceObject;
typedef Ptr<class CResourceGenerator> PResourceGenerator;

class CResourceGenerator: public Core::CObject
{
	__DeclareClassNoFactory;

public:

	//virtual ~CResourceGenerator() {}

	virtual const Core::CRTTI&	GetResultType() const = 0;
	virtual PResourceObject		Generate() = 0;
	//???Unload() - responsibility, ownership
};

}

#endif
