#pragma once
#include <Resources/ResourceLoader.h>

// Loads CDLOD terrain rendering data from DEM "cdlod" format

namespace Resources
{

class CCDLODDataLoader: public CResourceLoader
{
public:

	CCDLODDataLoader(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual Core::PObject		CreateResource(CStrID UID) override;
};

typedef Ptr<CCDLODDataLoader> PCDLODDataLoader;

}
