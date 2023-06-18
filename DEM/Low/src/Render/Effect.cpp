#include "Effect.h"
#include <Render/Texture.h>
#include <Render/Sampler.h>

namespace Render
{

CEffect::CEffect(EEffectType Type, PShaderParamTable MaterialParams, CShaderParamValues&& MaterialDefaults, std::map<CStrID, PTechnique>&& Techs)
	: _Type(Type)
	, _TechsByInputSet(std::move(Techs))
	, _MaterialParams(MaterialParams)
	, _MaterialDefaults(std::move(MaterialDefaults))
{
	for (const auto& [InputSet, Tech] : _TechsByInputSet)
	{
		Tech->_pOwner = this;
		if (Tech->GetName())
			_TechsByName[Tech->GetName()] = Tech;
	}
}
//---------------------------------------------------------------------

CEffect::~CEffect() = default;
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

CTexture* CEffect::GetResourceDefaultValue(CStrID ID) const
{
	auto It = _MaterialDefaults.ResourceValues.find(ID);
	return (It == _MaterialDefaults.ResourceValues.cend()) ? nullptr : It->second.Get();
}
//---------------------------------------------------------------------

CSampler* CEffect::GetSamplerDefaultValue(CStrID ID) const
{
	auto It = _MaterialDefaults.SamplerValues.find(ID);
	return (It == _MaterialDefaults.SamplerValues.cend()) ? nullptr : It->second.Get();
}
//---------------------------------------------------------------------

}
