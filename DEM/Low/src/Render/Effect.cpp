#include "Effect.h"
#include <Render/Texture.h>
#include <Render/Sampler.h>

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
	_TechsByInputSet[InputSet] = Tech;

	if (Tech->GetName()) _TechsByName[Tech->GetName()] = Tech;
}
//---------------------------------------------------------------------

const CTechnique* CEffect::GetTechByName(CStrID Name) const
{
	auto It = _TechsByName.find(Name);
	return (It == _TechsByName.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

const CTechnique* CEffect::GetTechByInputSet(CStrID InputSet) const
{
	auto It = _TechsByInputSet.find(InputSet);
	return (It == _TechsByInputSet.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

const CShaderConstValue* CEffect::GetConstantDefaultValue(CStrID ID) const
{
	auto It = _MaterialDefaults.ConstValues.find(ID);
	return (It == _MaterialDefaults.ConstValues.cend()) ? nullptr : &It->second;
}
//---------------------------------------------------------------------

PTexture CEffect::GetResourceDefaultValue(CStrID ID) const
{
	auto It = _MaterialDefaults.ResourceValues.find(ID);
	return (It == _MaterialDefaults.ResourceValues.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

PSampler CEffect::GetSamplerDefaultValue(CStrID ID) const
{
	auto It = _MaterialDefaults.SamplerValues.find(ID);
	return (It == _MaterialDefaults.SamplerValues.cend()) ? nullptr : It->second;
}
//---------------------------------------------------------------------

}