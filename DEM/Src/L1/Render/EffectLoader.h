#pragma once
#ifndef __DEM_L1_EFFECT_LOADER_H__
#define __DEM_L1_EFFECT_LOADER_H__

#include <Resources/ResourceLoader.h>

// Loads shader effect file in DEM (.eff) format

namespace IO
{
	class CBinaryReader;
}

namespace Render
{
	typedef Ptr<class CGPUDriver> PGPUDriver;
}

namespace Resources
{

class CEffectLoader: public CResourceLoader
{
protected:

	bool						LoadEffectParams(IO::CBinaryReader& Reader) const;
	bool						LoadEffectParamDefaultValues(IO::CBinaryReader& Reader) const;

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
