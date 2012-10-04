#pragma once
#ifndef __DEM_L1_RESOURCE_LOADER_H__
#define __DEM_L1_RESOURCE_LOADER_H__

#include <Resources/Resource.h>

// Interface for implementing specific resource loading algorithms.

namespace Resources
{

class CResourceLoader: public Core::CRefCounted
{
	DeclareRTTI;

	PResource Resource;

protected:

public:

	// Reload - force reloading from source
};

typedef Ptr<CResourceLoader> PResourceLoader;

}

#endif
