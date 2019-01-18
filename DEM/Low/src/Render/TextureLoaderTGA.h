#pragma once
#ifndef __DEM_L1_TEXTURE_LOADER_TGA_H__
#define __DEM_L1_TEXTURE_LOADER_TGA_H__

#include <Render/TextureLoader.h>

// Loads a texture in a Truevision Targa (.tga) format

namespace Resources
{

class CTextureLoaderTGA: public CTextureLoader
{
	__DeclareClass(CTextureLoaderTGA);

public:

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CTextureLoaderTGA> PTextureLoaderTGA;

}

#endif
