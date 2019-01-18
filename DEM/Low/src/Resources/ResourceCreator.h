#pragma once
#include <Data/RefCounted.h>
#include <Data/StringID.h>
#include <Core/RTTI.h>

// Resource creator is an interface to implement different resource object
// obtaining algorightms, whether by loading or procedural generation.
// Being attached to a CResource it allows to reload it on resource object lost.

namespace Resources
{
typedef Ptr<class CResourceObject> PResourceObject;
typedef Ptr<class IResourceCreator> PResourceCreator;

class IResourceCreator: public Data::CRefCounted
{
public:

	virtual ~IResourceCreator() {}

	virtual const Core::CRTTI&	GetResultType() const = 0;
	virtual PResourceObject		CreateResource(CStrID UID) = 0;
};

}
