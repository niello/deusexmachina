#pragma once
#include <Resources/ResourceLoader.h>

// Loads CSkinInfo (skin & skeleton data) from a native DEM 'skn' format

namespace Resources
{

class CSkinInfoLoaderSKN: public CResourceLoader
{
public:

	CSkinInfoLoaderSKN(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual Core::PObject		CreateResource(CStrID UID) override;
};

typedef Ptr<CSkinInfoLoaderSKN> PSkinInfoLoaderSKN;

}
