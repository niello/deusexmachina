#pragma once
#include <Resources/ResourceLoader.h>

// Loads a texture in a Truevision Targa (.tga) format

namespace Resources
{

class CTextureLoaderTGA: public CResourceLoader
{
public:

	CTextureLoaderTGA(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const DEM::Core::CRTTI&			GetResultType() const override;
	virtual DEM::Core::PObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CTextureLoaderTGA> PTextureLoaderTGA;

}
