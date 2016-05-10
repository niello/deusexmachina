#pragma once
#ifndef __DEM_L1_D3D9_TEXTURE_LOADER_DDS_H__
#define __DEM_L1_D3D9_TEXTURE_LOADER_DDS_H__

#include <Render/TextureLoader.h>

// Texture loaders are intended to load GPU texture resources. It is a class of
// different loaders each suited for a different image format and 3D API.

namespace Resources
{

class CD3D9TextureLoaderDDS: public CTextureLoader
{
	__DeclareClass(CD3D9TextureLoaderDDS);

public:

	// GPU, Access, MipCount (-1 for from-file or full)

	//virtual ~CD3D11TextureLoaderDDS() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CD3D9TextureLoaderDDS> PD3D9TextureLoaderDDS;

}

#endif
