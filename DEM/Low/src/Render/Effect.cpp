#include "Effect.h"
#include <Render/Sampler.h>
#include <Render/ShaderConstant.h>
#include <Render/Texture.h>

namespace Render
{
__ImplementClassNoFactory(Render::CEffect, Resources::CResourceObject);

CEffect::CEffect()
{
}
//---------------------------------------------------------------------

CEffect::~CEffect()
{
	SAFE_FREE(pMaterialConstDefaultValues);
}
//---------------------------------------------------------------------

PTexture CEffect::GetResourceDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultResources.FindIndex(ID);
	return Idx == INVALID_INDEX ? NULL : DefaultResources.ValueAt(Idx);
}
//---------------------------------------------------------------------

PSampler CEffect::GetSamplerDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultSamplers.FindIndex(ID);
	return Idx == INVALID_INDEX ? NULL : DefaultSamplers.ValueAt(Idx);
}
//---------------------------------------------------------------------

}