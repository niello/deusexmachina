#include <StdCfg.h>
#include "DEMShaderWrapper.h"

#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Render/Sampler.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMTexture.h>

#include <CEGUI/ShaderParameterBindings.h>
#include <glm/gtc/type_ptr.hpp>

namespace CEGUI
{

CDEMShaderWrapper::CDEMShaderWrapper(Render::PGPUDriver GPU, Render::CEffect& Effect, Render::PSampler LinearSampler)
	: _GPU(GPU)
	, _Effect(&Effect)
	, _LinearSampler(LinearSampler)
{
}
//---------------------------------------------------------------------

CDEMShaderWrapper::~CDEMShaderWrapper() = default;
//---------------------------------------------------------------------

void CDEMShaderWrapper::setInputSet(BlendMode BlendMode, bool Clipped, bool Opaque)
{
	// TODO CEGUI: use small opaque white texture for not textured geometry for batching everything together?
	static const CStrID TexturedRegularUnclipped("TexturedRegularUnclippedUI");
	static const CStrID TexturedPremultipliedUnclipped("TexturedPremultipliedUnclippedUI");
	static const CStrID TexturedOpaqueUnclipped("TexturedOpaqueUnclippedUI");
	static const CStrID TexturedRegularClipped("TexturedRegularClippedUI");
	static const CStrID TexturedPremultipliedClipped("TexturedPremultipliedClippedUI");
	static const CStrID TexturedOpaqueClipped("TexturedOpaqueClippedUI");
	static const CStrID ColoredRegularUnclipped("ColoredRegularUnclippedUI");
	static const CStrID ColoredPremultipliedUnclipped("ColoredPremultipliedUnclippedUI");
	static const CStrID ColoredOpaqueUnclipped("ColoredOpaqueUnclippedUI");
	static const CStrID ColoredRegularClipped("ColoredRegularClippedUI");
	static const CStrID ColoredPremultipliedClipped("ColoredPremultipliedClippedUI");
	static const CStrID ColoredOpaqueClipped("ColoredOpaqueClippedUI");

	CStrID NewInputSet;
	if (_LinearSampler)
	{
		// Textured materials
		if (Opaque)
			NewInputSet = Clipped ? TexturedOpaqueClipped : TexturedOpaqueUnclipped;
		else if (BlendMode == BlendMode::RttPremultiplied)
			NewInputSet = Clipped ? TexturedPremultipliedClipped : TexturedPremultipliedUnclipped;
		else
			NewInputSet = Clipped ? TexturedRegularClipped : TexturedRegularUnclipped;
	}
	else
	{
		// Colored materials
		if (Opaque)
			NewInputSet = Clipped ? ColoredOpaqueClipped : ColoredOpaqueUnclipped;
		else if (BlendMode == BlendMode::RttPremultiplied)
			NewInputSet = Clipped ? ColoredPremultipliedClipped : ColoredPremultipliedUnclipped;
		else
			NewInputSet = Clipped ? ColoredRegularClipped : ColoredRegularUnclipped;
	}

	if (_CurrInputSet == NewInputSet) return;

	_CurrInputSet = NewInputSet;

	auto It = _TechCache.find(NewInputSet);
	if (It == _TechCache.cend())
	{
		static const CStrID sidTextureID("BoundTexture");
		static const CStrID sidSamplerID("LinearSampler");
		static const CStrID sidWVP("WVP");
		static const CStrID sidAlphaPercentage("AlphaPercentage");

		const auto* pTech = _Effect->GetTechByInputSet(_CurrInputSet);
		n_assert_dbg(pTech);
		if (!pTech) return;

		Render::IResourceParam* pMainTextureParam = pTech->GetParamTable().GetResource(sidTextureID);
		Render::ISamplerParam* pLinearSamplerParam = pTech->GetParamTable().GetSampler(sidSamplerID);
		Render::CShaderConstantParam WVPParam = pTech->GetParamTable().GetConstant(sidWVP);
		Render::CShaderConstantParam AlphaPercentageParam = pTech->GetParamTable().GetConstant(sidAlphaPercentage);

		CTechCache NewCache
		{
			pTech,
			pMainTextureParam,
			pLinearSamplerParam,
			WVPParam,
			AlphaPercentageParam,
			Render::CShaderParamStorage(pTech->GetParamTable(), *_GPU)
		};

		It = _TechCache.emplace(NewInputSet, std::move(NewCache)).first;
	}
	
	_pCurrCache = &It->second;

	// FIXME: no multipass support for now, CEGUI does multiple passes in its effects
	UPTR LightCount = 0;
	_GPU->SetRenderState(_pCurrCache->pTech->GetPasses(LightCount)[0]);

	if (_pCurrCache && _pCurrCache->pLinearSamplerParam)
		_pCurrCache->pLinearSamplerParam->Apply(*_GPU, _LinearSampler.Get());
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::prepareForRendering(const ShaderParameterBindings* shaderParameterBindings)
{
	if (!_pCurrCache || !_pCurrCache->pTech) return;

	const ShaderParameterBindings::ShaderParameterBindingsMap& paramMap = shaderParameterBindings->getShaderParameterBindings();
	for (auto&& param : paramMap)
	{
		// Default CEGUI texture. Hardcoded inside CEGUI as "texture0".
		if (param.first == "texture0")
		{
			if (_pCurrCache->pMainTextureParam)
			{
				const CEGUI::ShaderParameterTexture* pPrm = static_cast<const CEGUI::ShaderParameterTexture*>(param.second);
				const CEGUI::CDEMTexture* pTex = static_cast<const CEGUI::CDEMTexture*>(pPrm->d_parameterValue);
				_pCurrCache->pMainTextureParam->Apply(*_GPU, pTex->getTexture());
			}
			continue;
		}
		else if (param.first == "WVP") // Cached for faster access
		{
			if (_pCurrCache->WVPParam)
			{
				const CEGUI::ShaderParameterMatrix* pPrm = static_cast<const CEGUI::ShaderParameterMatrix*>(param.second);
				_pCurrCache->Storage.SetRawConstant(_pCurrCache->WVPParam, glm::value_ptr(pPrm->d_parameterValue), sizeof(glm::mat4));
			}
			continue;
		}
		else if (param.first == "AlphaPercentage") // Cached for faster access
		{
			if (_pCurrCache->AlphaPercentageParam)
			{
				const CEGUI::ShaderParameterFloat* pPrm = static_cast<const CEGUI::ShaderParameterFloat*>(param.second);
				_pCurrCache->Storage.SetFloat(_pCurrCache->AlphaPercentageParam, pPrm->d_parameterValue);
			}
			continue;
		}

		auto Const = _pCurrCache->pTech->GetParamTable().GetConstant(CStrID(param.first.c_str()));
		if (!Const) continue;

		switch (param.second->getType())
		{
			case CEGUI::ShaderParamType::Matrix4X4:
			{
				const CEGUI::ShaderParameterMatrix* pPrm = static_cast<const CEGUI::ShaderParameterMatrix*>(param.second);
				_pCurrCache->Storage.SetRawConstant(Const, glm::value_ptr(pPrm->d_parameterValue), sizeof(glm::mat4));
				break;
			}
			case CEGUI::ShaderParamType::Float:
			{
				const CEGUI::ShaderParameterFloat* pPrm = static_cast<const CEGUI::ShaderParameterFloat*>(param.second);
				_pCurrCache->Storage.SetFloat(Const, pPrm->d_parameterValue);
				break;
			}
			case CEGUI::ShaderParamType::Int:
			{
				const CEGUI::ShaderParameterInt* pPrm = static_cast<const CEGUI::ShaderParameterInt*>(param.second);
				_pCurrCache->Storage.SetInt(Const, pPrm->d_parameterValue);
				break;
			}
			case CEGUI::ShaderParamType::Texture:
			{
				::Sys::Error("CDEMShaderWrapper::prepareForRendering() > CEGUI arbitrary texture not implemented\n");
				break;
			}
			default:
			{
				::Sys::Error("CDEMShaderWrapper::prepareForRendering() > unknown shader parameter type\n");
				break;
			}
		}
	}

	_pCurrCache->Storage.Apply();
}
//---------------------------------------------------------------------

}
