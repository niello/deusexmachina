#include <StdCfg.h>
#include "DEMShaderWrapper.h"

#include <Render/GPUDriver.h>
#include <Render/RenderStateDesc.h>
#include <Render/RenderState.h>
#include <Render/Shader.h>
#include <Render/ShaderMetadata.h>
#include <Render/ShaderConstant.h>
#include <Render/SamplerDesc.h>
#include <Render/Sampler.h>
#include <Render/ConstantBuffer.h>
#include <UI/CEGUI/DEMRenderer.h>
#include <UI/CEGUI/DEMTexture.h>

#include <CEGUI/ShaderParameterBindings.h>
#include <glm/gtc/type_ptr.hpp>

namespace CEGUI
{

CDEMShaderWrapper::CDEMShaderWrapper(CDEMRenderer& Owner, Render::PShader VS, Render::PShader PSRegular, Render::PShader PSOpaque)
	: Renderer(Owner)
{
	Render::CGPUDriver* pGPU = Owner.getGPUDriver();

	Render::CRenderStateDesc RSDesc;
	Render::CRenderStateDesc::CRTBlend& RTBlendDesc = RSDesc.RTBlend[0];
	RSDesc.SetDefaults();
	RSDesc.VertexShader = VS;
	RSDesc.PixelShader = PSRegular;
	RSDesc.Flags.Set(Render::CRenderStateDesc::Blend_RTBlendEnable << 0);
	RSDesc.Flags.Clear(Render::CRenderStateDesc::DS_DepthEnable |
		Render::CRenderStateDesc::DS_DepthWriteEnable |
		Render::CRenderStateDesc::Rasterizer_DepthClipEnable |
		Render::CRenderStateDesc::Rasterizer_Wireframe |
		Render::CRenderStateDesc::Rasterizer_CullFront |
		Render::CRenderStateDesc::Rasterizer_CullBack |
		Render::CRenderStateDesc::Blend_AlphaToCoverage |
		Render::CRenderStateDesc::Blend_Independent);

	// Regular shader with normal blend
	RTBlendDesc.SrcBlendArgAlpha = Render::BlendArg_InvDestAlpha;
	RTBlendDesc.DestBlendArgAlpha = Render::BlendArg_One;
	RTBlendDesc.SrcBlendArg = Render::BlendArg_SrcAlpha;
	RTBlendDesc.DestBlendArg = Render::BlendArg_InvSrcAlpha;

	// Unclipped
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	RegularUnclipped = pGPU->CreateRenderState(RSDesc);
	n_assert(RegularUnclipped.IsValidPtr());

	// Clipped
	RSDesc.Flags.Set(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	RegularClipped = pGPU->CreateRenderState(RSDesc);
	n_assert(RegularClipped.IsValidPtr());

	// Premultiplied alpha blend
	RTBlendDesc.SrcBlendArgAlpha = Render::BlendArg_One;
	RTBlendDesc.DestBlendArgAlpha = Render::BlendArg_InvSrcAlpha;
	RTBlendDesc.SrcBlendArg = Render::BlendArg_One;
	RTBlendDesc.DestBlendArg = Render::BlendArg_InvSrcAlpha;

	// Clipped
	PremultipliedClipped = pGPU->CreateRenderState(RSDesc);
	n_assert(PremultipliedClipped.IsValidPtr());

	// Unclipped
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	PremultipliedUnclipped = pGPU->CreateRenderState(RSDesc);
	n_assert(PremultipliedUnclipped.IsValidPtr());

	// Opaque
	RSDesc.PixelShader = PSOpaque;
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Blend_RTBlendEnable << 0);
	RSDesc.Flags.Set(Render::CRenderStateDesc::DS_DepthEnable |
		Render::CRenderStateDesc::DS_DepthWriteEnable);
	RSDesc.DepthFunc = Render::Cmp_Always;

	// Unclipped
	OpaqueUnclipped = pGPU->CreateRenderState(RSDesc);
	n_assert(OpaqueUnclipped.IsValidPtr());

	// Clipped
	RSDesc.Flags.Set(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	OpaqueClipped = pGPU->CreateRenderState(RSDesc);
	n_assert(OpaqueClipped.IsValidPtr());

	// NB: all pixel shaders must have compatible metadata
	pVSMeta = VS->GetMetadata();
	pPSMeta = PSRegular->GetMetadata();
}
//---------------------------------------------------------------------

CDEMShaderWrapper::~CDEMShaderWrapper()
{
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::setupParameterForShader(CStrID Name, Render::EShaderType ShaderType)
{
	const Render::IShaderMetadata* pMeta;
	switch (ShaderType)
	{
		case Render::ShaderType_Vertex:	pMeta = pVSMeta; break;
		case Render::ShaderType_Pixel:	pMeta = pPSMeta; break;
		default:						return;
	}

	if (!pMeta) return;

	Render::PShaderConstant Constant = pMeta->GetConstant(Name);
	if (!Constant || Constant->GetConstantBufferHandle() == INVALID_HANDLE) return;

	Render::HConstantBuffer hCB = Constant->GetConstantBufferHandle();
	Render::PConstantBuffer	CB;

	for (const auto& Rec : Constants)
	{
		if (Rec.Constant->GetConstantBufferHandle() == hCB)
		{
			CB = Rec.Buffer;
			break;
		}
	}

	if (!CB)
		CB = Renderer.getGPUDriver()->CreateConstantBuffer(hCB, Render::Access_GPU_Read | Render::Access_CPU_Write);

	CConstRecord Rec;
	Rec.Name = Name;
	Rec.Constant = Constant;
	Rec.ShaderType = ShaderType;
	Rec.Buffer = CB;

	Constants.push_back(std::move(Rec));
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::setupParameter(const char* pName)
{
	CStrID Name(pName);
	setupParameterForShader(Name, Render::ShaderType_Vertex);
	setupParameterForShader(Name, Render::ShaderType_Pixel);
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::setupMainTexture(const char* pTextureName, const char* pSamplerName)
{
	if (!pPSMeta) return;

	hTexture = pPSMeta->GetResourceHandle(CStrID(pTextureName));
	hLinearSampler = pPSMeta->GetSamplerHandle(CStrID(pSamplerName));

	if (!LinearSampler)
	{
		Render::CSamplerDesc SampDesc;
		SampDesc.SetDefaults();
		SampDesc.AddressU = Render::TexAddr_Clamp;
		SampDesc.AddressV = Render::TexAddr_Clamp;
		SampDesc.Filter = Render::TexFilter_MinMagMip_Linear;
		LinearSampler = Renderer.getGPUDriver()->CreateSampler(SampDesc);
		n_assert(LinearSampler.IsValidPtr());
	}
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::bindRenderState(BlendMode BlendMode, bool Clipped, bool Opaque) const
{
	Render::CGPUDriver* pGPU = Renderer.getGPUDriver();

	if (Opaque)
		pGPU->SetRenderState(Clipped ? OpaqueClipped : OpaqueUnclipped);
	else if (BlendMode == BlendMode::RttPremultiplied)
		pGPU->SetRenderState(Clipped ? PremultipliedClipped : PremultipliedUnclipped);
	else
		pGPU->SetRenderState(Clipped ? RegularClipped : RegularUnclipped);
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::prepareForRendering(const ShaderParameterBindings* shaderParameterBindings)
{
	// Shader is set in bindRenderState

	Render::CGPUDriver* pGPU = Renderer.getGPUDriver();

	const ShaderParameterBindings::ShaderParameterBindingsMap& paramMap = shaderParameterBindings->getShaderParameterBindings();
	for (auto&& param : paramMap)
	{
		// Default CEGUI texture. Hardcoded inside CEGUI as "texture0".
		if (param.first == "texture0")
		{
			if (hTexture && LinearSampler)
			{
				const CEGUI::ShaderParameterTexture* pPrm = static_cast<const CEGUI::ShaderParameterTexture*>(param.second);
				const CEGUI::CDEMTexture* pTex = static_cast<const CEGUI::CDEMTexture*>(pPrm->d_parameterValue);
				pGPU->BindResource(Render::ShaderType_Pixel, hTexture, pTex->getTexture());
				pGPU->BindSampler(Render::ShaderType_Pixel, hLinearSampler, LinearSampler.Get());
			}

			continue;
		}

		// Constants
		for (const auto& Rec : Constants)
		{
			// Set in each shader where this constant exists by name, don't break after the first name match
			if (Rec.Name != param.first.c_str()) continue;

			if (!Rec.Buffer->IsInWriteMode())
			{
				pGPU->BindConstantBuffer(Rec.ShaderType, Rec.Constant->GetConstantBufferHandle(), Rec.Buffer.Get());
				pGPU->BeginShaderConstants(*Rec.Buffer.Get());
			}

			switch (param.second->getType())
			{
				case CEGUI::ShaderParamType::Matrix4X4:
				{
					const CEGUI::ShaderParameterMatrix* pPrm = static_cast<const CEGUI::ShaderParameterMatrix*>(param.second);
					Rec.Constant->SetRawValue(*Rec.Buffer.Get(), glm::value_ptr(pPrm->d_parameterValue), sizeof(glm::mat4));
					break;
				}
				case CEGUI::ShaderParamType::Float:
				{
					const CEGUI::ShaderParameterFloat* pPrm = static_cast<const CEGUI::ShaderParameterFloat*>(param.second);
					Rec.Constant->SetFloat(*Rec.Buffer.Get(), &pPrm->d_parameterValue);
					break;
				}
				case CEGUI::ShaderParamType::Int:
				{
					const CEGUI::ShaderParameterInt* pPrm = static_cast<const CEGUI::ShaderParameterInt*>(param.second);
					Rec.Constant->SetSInt(*Rec.Buffer.Get(), pPrm->d_parameterValue);
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
	}

	for (const auto& Rec : Constants)
	{
		if (Rec.Buffer->IsInWriteMode())
			pGPU->CommitShaderConstants(*Rec.Buffer.Get());
	}
}
//---------------------------------------------------------------------

}
