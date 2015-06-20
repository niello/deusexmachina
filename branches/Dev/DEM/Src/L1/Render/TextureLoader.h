#pragma once
#ifndef __DEM_L1_TEXTURE_LOADER_H__
#define __DEM_L1_TEXTURE_LOADER_H__

#include <Resources/ResourceLoader.h>

// Texture loaders are intended to load GPU texture resources. It is a class of
// different loaders each suited for a different image format and 3D API.

namespace Resources
{

class CTextureLoader: public CResourceLoader
{
	__DeclareClassNoFactory;

public:

	// GPU, Access, MipCount (-1 for from-file or full)

	virtual ~CTextureLoader() {}

	virtual const Core::CRTTI&	GetResultType() const;
};

typedef Ptr<CTextureLoader> PTextureLoader;

}

#endif
