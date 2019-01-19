#pragma once
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
public:

	//UPTR				DesiredMipCount;
};

typedef Ptr<CTextureLoader> PTextureLoader;

}
