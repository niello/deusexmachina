#pragma once
#include <Resources/ResourceLoader.h>

// Loads a texture in a Direct Draw Surface (.dds) format

namespace Resources
{

class CTextureLoaderDDS: public CResourceLoader
{
public:

	CTextureLoaderDDS(CResourceManager& ResourceManager) : CResourceLoader(ResourceManager) {}

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CTextureLoaderDDS> PTextureLoaderDDS;

}
