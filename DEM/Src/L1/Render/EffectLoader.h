#pragma once
#ifndef __DEM_L1_EFFECT_LOADER_H__
#define __DEM_L1_EFFECT_LOADER_H__

#include <Resources/ResourceLoader.h>
#include <Render/RenderFwd.h>

// Loads shader effect file in DEM (.eff) format

namespace IO
{
	class CBinaryReader;
}

namespace Resources
{

class CEffectLoader: public CResourceLoader
{
protected:

	struct CLoadedParam
	{
		CStrID					ID;
		U8						Type;
		U8						ShaderType;
		HHandle					Handle;			// May use union if handles become type-incompatible
		Render::HConstBuffer	BufferHandle;	// For constants only
		U8						ConstType;		// For constants only
		U32						SizeInBytes;	// For constants only
	};

	static bool					LoadEffectParams(IO::CBinaryReader& Reader, Render::PShaderLibrary ShaderLibrary, CArray<CLoadedParam>& Out);
	static bool					LoadEffectParamValues(IO::CBinaryReader& Reader, Render::CEffect& Effect, Render::PGPUDriver GPU);
	static bool					SkipEffectParams(IO::CBinaryReader& Reader);

public:

	static Render::PTexture		LoadTextureValue(IO::CBinaryReader& Reader, Render::PGPUDriver GPU);
	static Render::PSampler		LoadSamplerValue(IO::CBinaryReader& Reader, Render::PGPUDriver GPU);

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
