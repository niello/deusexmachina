#pragma once
#include <Resources/ResourceLoader.h>

// Loads terrain collision shape from DEM "cdlod" format

namespace Resources
{

class CCollisionLoaderCDLOD: public CResourceLoader
{
public:

	CCollisionLoaderCDLOD(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CCollisionLoaderCDLOD> PCollisionLoaderCDLOD;

}
