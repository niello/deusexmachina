#pragma once
#include <Resources/ResourceLoader.h>

// Loads terrain collision shape from DEM "cdlod" format

namespace Resources
{

class CCollisionLoaderCDLOD: public CResourceLoader
{
public:

	CCollisionLoaderCDLOD(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override;
	virtual Core::PObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CCollisionLoaderCDLOD> PCollisionLoaderCDLOD;

}
