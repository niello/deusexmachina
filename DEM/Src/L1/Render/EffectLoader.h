#pragma once
#ifndef __DEM_L1_EFFECT_LOADER_H__
#define __DEM_L1_EFFECT_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads shader effect file in DEM (.eff) format

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CEffectLoader: public CResourceLoader
{

public:

	Render::PGPUDriver GPU;

	virtual ~CEffectLoader() {}

	virtual const Core::CRTTI&	GetResultType() const;
	virtual bool				IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual bool				Load(CResource& Resource);
};

typedef Ptr<CEffectLoader> PEffectLoader;

}

#endif
