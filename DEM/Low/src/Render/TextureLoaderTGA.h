#pragma once
#include <Render/TextureLoader.h>

// Loads a texture in a Truevision Targa (.tga) format

namespace Resources
{

class CTextureLoaderTGA: public CTextureLoader
{
public:

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CTextureLoaderTGA> PTextureLoaderTGA;

}
