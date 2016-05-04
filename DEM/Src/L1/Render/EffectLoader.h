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
	};

	bool						LoadEffectParams(IO::CBinaryReader& Reader, CArray<CLoadedParam>& Out) const;
	bool						LoadEffectParamDefaultValues(IO::CBinaryReader& Reader, Render::CEffect& Effect) const;

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
