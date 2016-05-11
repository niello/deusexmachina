#pragma once
#ifndef __DEM_L1_TEXTURE_LOADER_DDS_H__
#define __DEM_L1_TEXTURE_LOADER_DDS_H__

#include <Render/TextureLoader.h>

// Loads a texture in a Direct Draw Surface (.dds) format

namespace Resources
{

class CTextureLoaderDDS: public CTextureLoader
{
	__DeclareClass(CTextureLoaderDDS);

public:

	//virtual ~CTextureLoaderDDS() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CTextureLoaderDDS> PTextureLoaderDDS;

}

#endif
