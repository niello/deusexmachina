#include <StdCfg.h>
#include "DEMShaderWrapper.h"

#include <Render/GPUDriver.h>
#include <Render/Effect.h>
#include <Render/ShaderParamStorage.h>
#include <Render/ShaderParamTable.h>
#include <Render/SamplerDesc.h>
#include <Render/Sampler.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMTexture.h>

#include <CEGUI/ShaderParameterBindings.h>
#include <glm/gtc/type_ptr.hpp>

namespace CEGUI
{

CDEMShaderWrapper::CDEMShaderWrapper(CDEMRenderer& Owner, Render::CEffect& Effect)
	: _Renderer(Owner)
	, _Effect(&Effect)
{
	Render::CSamplerDesc SampDesc;
	SampDesc.SetDefaults();
	SampDesc.AddressU = Render::TexAddr_Clamp;
	SampDesc.AddressV = Render::TexAddr_Clamp;
	SampDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	_LinearSampler = _Renderer.getGPUDriver()->CreateSampler(SampDesc);
	n_assert(_LinearSampler.IsValidPtr());
}
//---------------------------------------------------------------------

CDEMShaderWrapper::~CDEMShaderWrapper() = default;
//---------------------------------------------------------------------

void CDEMShaderWrapper::setInputSet(BlendMode BlendMode, bool Clipped, bool Opaque)
{
	static const CStrID RegularUnclipped("RegularUnclippedUI");
	static const CStrID PremultipliedUnclipped("PremultipliedUnclippedUI");
	static const CStrID OpaqueUnclipped("OpaqueUnclippedUI");
	static const CStrID RegularClipped("RegularClippedUI");
	static const CStrID PremultipliedClipped("PremultipliedClippedUI");
	static const CStrID OpaqueClipped("OpaqueClippedUI");
	if (Opaque)
		_CurrInputSet = Clipped ? OpaqueClipped : OpaqueUnclipped;
	else if (BlendMode == BlendMode::RttPremultiplied)
		_CurrInputSet = Clipped ? PremultipliedClipped : PremultipliedUnclipped;
	else
		_CurrInputSet = Clipped ? RegularClipped : RegularUnclipped;
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::prepareForRendering(const ShaderParameterBindings* shaderParameterBindings)
{
	const auto* pTech = _Effect->GetTechByInputSet(_CurrInputSet);
	if (!pTech) return;

	Render::CGPUDriver* pGPU = _Renderer.getGPUDriver();

	Render::CShaderParamStorage Storage(pTech->GetParamTable(), *pGPU);

	const ShaderParameterBindings::ShaderParameterBindingsMap& paramMap = shaderParameterBindings->getShaderParameterBindings();
	for (auto&& param : paramMap)
	{
		// Default CEGUI texture. Hardcoded inside CEGUI as "texture0".
		if (param.first == "texture0")
		{
			static const CStrID sidTextureID("BoundTexture");
			static const CStrID sidSamplerID("LinearSampler");

			if (auto TextureParam = pTech->GetParamTable().GetResource(sidTextureID))
			{
				const CEGUI::ShaderParameterTexture* pPrm = static_cast<const CEGUI::ShaderParameterTexture*>(param.second);
				const CEGUI::CDEMTexture* pTex = static_cast<const CEGUI::CDEMTexture*>(pPrm->d_parameterValue);
				TextureParam->Apply(*pGPU, pTex->getTexture());

				if (auto SamplerParam = pTech->GetParamTable().GetSampler(sidSamplerID))
					SamplerParam->Apply(*pGPU, _LinearSampler.Get());
			}

			continue;
		}

		auto Const = pTech->GetParamTable().GetConstant(CStrID(param.first.c_str()));
		if (!Const) continue;

		switch (param.second->getType())
		{
			case CEGUI::ShaderParamType::Matrix4X4:
			{
				const CEGUI::ShaderParameterMatrix* pPrm = static_cast<const CEGUI::ShaderParameterMatrix*>(param.second);
				Storage.SetRawConstant(Const, glm::value_ptr(pPrm->d_parameterValue), sizeof(glm::mat4));
				break;
			}
			case CEGUI::ShaderParamType::Float:
			{
				const CEGUI::ShaderParameterFloat* pPrm = static_cast<const CEGUI::ShaderParameterFloat*>(param.second);
				Storage.SetFloat(Const, pPrm->d_parameterValue);
				break;
			}
			case CEGUI::ShaderParamType::Int:
			{
				const CEGUI::ShaderParameterInt* pPrm = static_cast<const CEGUI::ShaderParameterInt*>(param.second);
				Storage.SetInt(Const, pPrm->d_parameterValue);
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

	Storage.Apply();
}
//---------------------------------------------------------------------

}
