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

	//virtual ~CTextureLoaderTGA() {}

	virtual const Core::CRTTI&			GetResultType() const;
	virtual bool						IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_RANDOM; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CTextureLoaderTGA> PTextureLoaderTGA;

}

#endif
