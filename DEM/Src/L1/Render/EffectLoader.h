#pragma once
#ifndef __DEM_L1_EFFECT_LOADER_H__
#define __DEM_L1_EFFECT_LOADER_H__

#include <Resources/ResourceLoader.h>
#include <Render/RenderFwd.h>
#include <Data/FixedArray.h>

// Loads shader effect file in DEM (.eff) format

namespace IO
{
	class CBinaryReader;
}

namespace Resources
{

class CEffectLoader: public CResourceLoader
{
public:

	Render::PGPUDriver		GPU;
	Render::PShaderLibrary	ShaderLibrary;

	virtual ~CEffectLoader() {}

	virtual PResourceLoader				Clone();
	virtual const Core::CRTTI&			GetResultType() const;
	virtual bool						IsProvidedDataValid() const { OK; } //!!!implement properly!
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CEffectLoader> PEffectLoader;

}

#endif
