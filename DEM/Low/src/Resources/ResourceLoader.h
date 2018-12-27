#pragma once
#ifndef __DEM_L1_RESOURCE_LOADER_H__
#define __DEM_L1_RESOURCE_LOADER_H__

#include <Core/Object.h>
#include <IO/IOFwd.h>

// Resource loader incapsulates an algorithm of loading a particular resource
// object type from a particular data format. It also stores all data necessary
// to create and initialize a resource object, if this data is not provided by
// a data format itself. Loader, being attached to a CResource, allows to reload
// it automatically on resource lost. Derive from this class to implement different
// resource object loader groups and loading algorithms.

namespace Resources
{
typedef Ptr<class CResourceObject> PResourceObject;
typedef Ptr<class CResourceLoader> PResourceLoader;

class CResourceLoader: public Core::CObject
{
	__DeclareClassNoFactory;

public:

	//virtual ~CResourceLoader() {}

	virtual PResourceLoader				Clone() { return this; } // Reimplement for loaders with state
	virtual const Core::CRTTI&			GetResultType() const = 0;
	virtual bool						IsProvidedDataValid() const = 0;
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_DEFAULT; }
	virtual PResourceObject				Load(IO::CStream& Stream) = 0;
	//???Unload() - responsibility, ownership
};

}

#endif
