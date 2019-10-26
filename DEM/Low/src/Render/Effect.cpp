#include "Effect.h"
#include <Render/GPUDriver.h>
#include <Render/Sampler.h>
#include <Render/ShaderConstant.h>
#include <Render/Texture.h>
#include <Render/RenderState.h>
#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
#include <Render/Shader.h>
#include <Render/ShaderMetadata.h>
#include <IO/BinaryReader.h>
#include <Data/StringUtils.h>

namespace Render
{

CEffect::CEffect(EEffectType Type, PShaderParamTable MaterialParams, CShaderParamValues&& MaterialDefaults)
	: _Type(Type)
	, _MaterialParams(MaterialParams)
	, _MaterialDefaults(std::move(MaterialDefaults))
{
}
//---------------------------------------------------------------------

CEffect::~CEffect()
{
}
//---------------------------------------------------------------------

void CEffect::SetTechnique(CStrID InputSet, PTechnique Tech)
{
}
//---------------------------------------------------------------------

const CTechnique* CEffect::GetTechByName(CStrID Name) const
{
	IPTR Idx = TechsByName.FindIndex(Name);
	return Idx == INVALID_INDEX ? nullptr : TechsByName.ValueAt(Idx).Get();
}
//---------------------------------------------------------------------

const CTechnique* CEffect::GetTechByInputSet(CStrID InputSet) const
{
	IPTR Idx = TechsByInputSet.FindIndex(InputSet);
	return Idx == INVALID_INDEX ? nullptr : TechsByInputSet.ValueAt(Idx).Get();
}
//---------------------------------------------------------------------

void* CEffect::GetConstantDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultConsts.FindIndex(ID);
	return Idx == INVALID_INDEX ? nullptr : DefaultConsts.ValueAt(Idx);
}
//---------------------------------------------------------------------

PTexture CEffect::GetResourceDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultResources.FindIndex(ID);
	return Idx == INVALID_INDEX ? nullptr : DefaultResources.ValueAt(Idx);
}
//---------------------------------------------------------------------

PSampler CEffect::GetSamplerDefaultValue(CStrID ID) const
{
	IPTR Idx = DefaultSamplers.FindIndex(ID);
	return Idx == INVALID_INDEX ? nullptr : DefaultSamplers.ValueAt(Idx);
}
//---------------------------------------------------------------------

}