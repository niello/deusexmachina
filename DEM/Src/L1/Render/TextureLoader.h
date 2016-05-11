#pragma once
#ifndef __DEM_L1_TEXTURE_LOADER_H__
#define __DEM_L1_TEXTURE_LOADER_H__

#include <Resources/ResourceLoader.h>

// Texture loaders are intended to load GPU texture resources. It is a class of
// different loaders each suited for a different image format and 3D API.

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CTextureLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	Render::PGPUDriver	GPU;
	//UPTR				AccessFlags;		//!!!uncomment and define to GPU_Read in a _default_ loader!
	//UPTR				DesiredMipCount;

	//virtual ~CTextureLoader() {}

	//virtual const Core::CRTTI&	GetResultType() const;
};

typedef Ptr<CTextureLoader> PTextureLoader;

}

#endif
