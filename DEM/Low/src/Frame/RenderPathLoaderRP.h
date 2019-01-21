#pragma once
#include <Resources/ResourceLoader.h>

// Loads a render path from DEM (.rp) format

namespace Resources
{

class CRenderPathLoaderRP: public CResourceLoader
{
public:

	CRenderPathLoaderRP(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CRenderPathLoaderRP> PRenderPathLoaderRP;

}
