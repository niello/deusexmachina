#pragma once
#ifndef __DEM_L1_RESOURCE_LOADER_H__
#define __DEM_L1_RESOURCE_LOADER_H__

#include <Core/Object.h>
#include <Data/StringID.h>

// Resource loader incapsulates an algorithm of loading a particular resource
// object type from a particular data format. It also stores all data necessary
// to create and initialize a resource object, if this data is not provided by
// a data format itself. Derive from this class to implement different resource
// object loader groups and loading algorithms.

namespace Resources
{

class CResourceLoader: public Core::CObject
{
	__DeclareClassNoFactory;

public:

	virtual ~CResourceLoader() {}

	// Get RTTI of resulting object
	// Check data existence and format validity without loading
	// Load (read data from URI etc), provided a resource ptr to fill //???control (a)sync in IO server or where?
};

typedef Ptr<CResourceLoader> PResourceLoader;

}

#endif
