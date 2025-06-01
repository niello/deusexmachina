#pragma once
#include <Resources/ResourceLoader.h>

// Loads a texture in a Direct Draw Surface (.dds) format

namespace Resources
{

class CTextureLoaderDDS: public CResourceLoader
{
public:

	CTextureLoaderDDS(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI& GetResultType() const override;
	virtual DEM::Core::PObject      CreateResource(CStrID UID) override;
};

typedef Ptr<CTextureLoaderDDS> PTextureLoaderDDS;

}
