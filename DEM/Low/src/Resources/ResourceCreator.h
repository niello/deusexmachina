#pragma once
#include <Core/Object.h>
#include <Data/StringID.h>

// Resource creator is an interface to different implementations of loading
// and procedural generation algorithms that produce a resource object.
// Being attached to a CResource it allows to reload it.

namespace Resources
{
typedef Ptr<class IResourceCreator> PResourceCreator;

class IResourceCreator : public Data::CRefCounted
{
public:

	virtual const DEM::Core::CRTTI&	GetResultType() const = 0;
	virtual DEM::Core::PObject		CreateResource(CStrID UID) = 0;
};

}
