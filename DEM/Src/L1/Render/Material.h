#pragma once
#ifndef __DEM_L1_RENDER_MATERIAL_H__
#define __DEM_L1_RENDER_MATERIAL_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Data/FixedArray.h>

// Material defines parameters of an effect to achieve some visual properties of object rendered.
// Material can be shared among render objects, providing the same shaders and shader parameters to all of them.

namespace Resources
{
	class CMaterialLoader;
}

namespace Render
{
typedef Ptr<class CMaterial> PMaterial;
typedef Ptr<class CEffect> PEffect;
class CTechnique;
class CGPUDriver;

class CMaterial: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

	struct CResourceRec
	{
		EShaderType		ShaderType;
		HResource		Handle;
		PTexture		Resource;
	};

	struct CSamplerRec
	{
		EShaderType		ShaderType;
		HSampler		Handle;
		PSampler		Sampler;
	};

	PEffect							Effect;
	CFixedArray<CConstBufferRecord>	ConstBuffers;
	CFixedArray<CResourceRec>		Resources;
	CFixedArray<CSamplerRec>		Samplers;

	friend class Resources::CMaterialLoader;

public:

	//virtual ~CMaterial();

	bool			Apply(CGPUDriver& GPU) const;

	virtual bool	IsResourceValid() const { return Effect.IsValidPtr(); }
	CEffect*		GetEffect() const { return Effect.GetUnsafe(); }
};

}

#endif
