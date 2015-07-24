#pragma once
#ifndef __DEM_L1_RESOURCE_LOADER_H__
#define __DEM_L1_RESOURCE_LOADER_H__

#include <Core/Object.h>
#include <Data/StringID.h>

// Resource loader incapsulates an algorithm of loading a particular resource
// object type from a particular data format. It also stores all data necessary
// to create and initialize a resource object, if this data is not provided by
// a data format itself. Loader, being attached to a CResource, allows to reload
// it automatically on resource lost. Derive from this class to implement different
// resource object loader groups and loading algorithms.

namespace Resources
{
typedef Ptr<class CResource> PResource;

class CResourceLoader: public Core::CObject
{
	__DeclareClassNoFactory;

public:

	//virtual ~CResourceLoader() {}

	virtual const Core::CRTTI&	GetResultType() const = 0;
	virtual bool				IsProvidedDataValid() const = 0;
	virtual bool				Load(CResource& Resource) = 0; //???assert resource is NotLoaded? //???async? //!!!call Mgr->LoadResource!
	//???Unload() - responsibility, ownership
};

typedef Ptr<CResourceLoader> PResourceLoader;

}

#endif
