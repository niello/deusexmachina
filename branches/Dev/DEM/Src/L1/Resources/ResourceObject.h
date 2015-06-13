#pragma once
#ifndef __DEM_L1_RESOURCE_OBJECT_H__
#define __DEM_L1_RESOURCE_OBJECT_H__

#include <Core/Object.h>
#include <Data/StringID.h>

// Resource object represents an actual data object (asset) used by some subsystem,
// whereas a CResource is only a management container for them. All specific resources
// must be derived from this class. Resource object is always a CObject as it must provide
// an RTTI and a reference counting. Unfortunately, we can't derive resource objects from
// a CObject directly, because resource management wants to know whether an actual object
// is valid, but validity status may change outside a resource manager scope, as with lost
// VRAM resources in D3D9. CResourceObject provides a validity query and mb some other API.

namespace Resources
{

class CResourceObject: public Core::CObject
{
	__DeclareClassNoFactory;

public:

	virtual ~CResourceObject() {}

	virtual bool	IsResourceValid() = 0;
};

typedef Ptr<CResourceObject> PResourceObject;

}

#endif
