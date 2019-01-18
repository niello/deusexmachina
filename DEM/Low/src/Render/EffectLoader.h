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

	virtual const Core::CRTTI&			GetResultType() const override;
	virtual PResourceObject				CreateResource(CStrID UID) override;
};

typedef Ptr<CEffectLoader> PEffectLoader;

}

#endif
