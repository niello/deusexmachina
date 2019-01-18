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

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CTextureLoaderDDS> PTextureLoaderDDS;

}

#endif
