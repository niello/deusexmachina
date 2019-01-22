#pragma once
#include <Resources/ResourceLoader.h>

// Loads a collision shape from HRD or PRM

namespace Resources
{

class CCollisionShapeLoaderPRM: public CResourceLoader
{
public:

	CCollisionShapeLoaderPRM(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual PResourceObject		CreateResource(CStrID UID) override;
};

typedef Ptr<CCollisionShapeLoaderPRM> PCollisionShapeLoaderPRM;

}
