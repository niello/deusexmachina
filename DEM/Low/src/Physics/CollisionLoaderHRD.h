#pragma once
#include <Resources/ResourceLoader.h>

// Loads collision shape from DEM general purpose JSON-like "hrd" format

namespace Resources
{

class CCollisionLoaderHRD: public CResourceLoader
{
public:

	CCollisionLoaderHRD(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override;
	virtual PResourceObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CCollisionLoaderHRD> PCollisionLoaderHRD;

}
