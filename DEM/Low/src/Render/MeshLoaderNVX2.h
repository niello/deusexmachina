#pragma once
#include <Resources/ResourceLoader.h>

// Loads CMesh from The Nebula Device 2 "nvx2" format

namespace Resources
{

class CMeshLoaderNVX2: public CResourceLoader
{
public:

	CMeshLoaderNVX2(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI&	GetResultType() const override;
	virtual DEM::Core::PObject		CreateResource(CStrID UID) override;
};

typedef Ptr<CMeshLoaderNVX2> PMeshLoaderNVX2;

}
