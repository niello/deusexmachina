#pragma once
#include <Resources/ResourceLoader.h>

// Loads navigation mesh from DEM "nm" format

namespace Resources
{

class CNavMeshLoaderNM: public CResourceLoader
{
public:

	CNavMeshLoaderNM(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI& GetResultType() const override;
	virtual PResourceObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CNavMeshLoaderNM> PNavMeshLoaderNM;

}
