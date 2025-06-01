#pragma once
#include <Resources/ResourceLoader.h>

// Loads CMesh from native DEM "msh" format

namespace Resources
{

class CMeshLoaderMSH: public CResourceLoader
{
public:

	CMeshLoaderMSH(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject    CreateResource(CStrID UID) override;
};

typedef Ptr<CMeshLoaderMSH> PMeshLoaderMSH;

}
