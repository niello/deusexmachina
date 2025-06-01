#pragma once
#include <Resources/ResourceLoader.h>

// Loads navigation mesh from DEM "nm" format

namespace Resources
{

class CNavMeshLoaderNM: public CResourceLoader
{
public:

	CNavMeshLoaderNM(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CNavMeshLoaderNM> PNavMeshLoaderNM;

}
