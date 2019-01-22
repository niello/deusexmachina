#pragma once
#include <Resources/ResourceLoader.h>

// Loads CDLOD terrain rendering data from DEM "cdlod" format

namespace Resources
{

class CTextureLoaderCDLOD: public CResourceLoader
{
public:

	CTextureLoaderCDLOD(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI&	GetResultType() const override;
	virtual PResourceObject		CreateResource(CStrID UID) override;
};

typedef Ptr<CTextureLoaderCDLOD> PTextureLoaderCDLOD;

}
