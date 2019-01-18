#pragma once
#ifndef __DEM_L1_EFFECT_LOADER_H__
#define __DEM_L1_EFFECT_LOADER_H__

#include <Resources/ResourceCreator.h>
#include <Render/RenderFwd.h>
#include <Data/FixedArray.h>

// Loads shader effect file in DEM (.eff) format

namespace IO
{
	class CBinaryReader;
}

namespace Resources
{

class CEffectLoader: public IResourceCreator
{
public:

	Render::PGPUDriver		GPU;
	Render::PShaderLibrary	ShaderLibrary;

	virtual ~CEffectLoader();

	virtual PResourceLoader				Clone();
	virtual const Core::CRTTI&			GetResultType() const;
	virtual IO::EStreamAccessPattern	GetStreamAccessPattern() const { return IO::SAP_SEQUENTIAL; }
	virtual PResourceObject				Load(IO::CStream& Stream);
};

typedef Ptr<CEffectLoader> PEffectLoader;

}

#endif
