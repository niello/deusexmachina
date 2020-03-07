#pragma once
#include <Resources/ResourceCreator.h>

// A subclass of resource creators which loads resource data from a stream.
// Path is incorporated into a resource UID. Resource manager provides a
// stream for UID.

namespace Resources
{
class CResourceManager;

class CResourceLoader: public IResourceCreator
{
protected:

	CResourceManager& _ResMgr;

public:

	CResourceLoader(CResourceManager& ResourceManager) : _ResMgr(ResourceManager) {}

	virtual const Core::CRTTI&	GetResultType() const = 0;
	virtual PResourceObject		CreateResource(CStrID UID) = 0;
};

}
