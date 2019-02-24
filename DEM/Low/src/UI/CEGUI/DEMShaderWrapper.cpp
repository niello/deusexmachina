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

#include <CEGUI/ShaderParameterBindings.h>

namespace CEGUI
{

CDEMShaderWrapper::CDEMShaderWrapper(CDEMRenderer& Owner, Render::PShader VS, Render::PShader PSRegular, Render::PShader PSOpaque)
	: Renderer(Owner)
{
	Render::CGPUDriver* pGPU = Owner.getGPUDriver();

	//=================================================================
	// Render states
	//=================================================================

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

	// Normal blend
	RTBlendDesc.SrcBlendArgAlpha = Render::BlendArg_InvDestAlpha;
	RTBlendDesc.DestBlendArgAlpha = Render::BlendArg_One;
	RTBlendDesc.SrcBlendArg = Render::BlendArg_SrcAlpha;
	RTBlendDesc.DestBlendArg = Render::BlendArg_InvSrcAlpha;

	// Unclipped
	RSDesc.Flags.Clear(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	NormalUnclipped = pGPU->CreateRenderState(RSDesc);
	n_assert(NormalUnclipped.IsValidPtr());

	// Clipped
	RSDesc.Flags.Set(Render::CRenderStateDesc::Rasterizer_ScissorEnable);

	NormalClipped = pGPU->CreateRenderState(RSDesc);
	n_assert(NormalClipped.IsValidPtr());

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

	//=================================================================
	// Shader constants
	//=================================================================

	//???or universal external? CEffectConst etc?

	const Render::IShaderMetadata* pVSMeta = VS->GetMetadata();
	const Render::IShaderMetadata* pPSMeta = PSRegular->GetMetadata();

	ConstWorldMatrix = pVSMeta->GetConstant(pVSMeta->GetConstantHandle(CStrID("WorldMatrix")));
	n_assert(ConstWorldMatrix.IsValidPtr());
	hWMCB = ConstWorldMatrix->GetConstantBufferHandle();
	n_assert(hWMCB != INVALID_HANDLE);

	ConstProjMatrix = pVSMeta->GetConstant(pVSMeta->GetConstantHandle(CStrID("ProjectionMatrix")));
	n_assert(ConstProjMatrix.IsValidPtr());
	hPMCB = ConstProjMatrix->GetConstantBufferHandle();
	n_assert(hPMCB != INVALID_HANDLE);

	WMCB = pGPU->CreateConstantBuffer(hWMCB, Render::Access_GPU_Read | Render::Access_CPU_Write);
	if (hWMCB == hPMCB) PMCB = WMCB;
	else PMCB = pGPU->CreateConstantBuffer(hPMCB, Render::Access_GPU_Read | Render::Access_CPU_Write);

	hTexture = pPSMeta->GetResourceHandle(CStrID("BoundTexture"));
	hLinearSampler = pPSMeta->GetSamplerHandle(CStrID("LinearSampler"));

	Render::CSamplerDesc SampDesc;
	SampDesc.SetDefaults();
	SampDesc.AddressU = Render::TexAddr_Clamp;
	SampDesc.AddressV = Render::TexAddr_Clamp;
	SampDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	LinearSampler = pGPU->CreateSampler(SampDesc);
	n_assert(LinearSampler.IsValidPtr());
}
//---------------------------------------------------------------------

CDEMShaderWrapper::~CDEMShaderWrapper()
{
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::bindRenderState(BlendMode BlendMode, bool Clipped, bool Opaque)
{
	Render::CGPUDriver* pGPU = Renderer.getGPUDriver();

	if (Opaque)
	{
		pGPU->SetRenderState(Clipped ? OpaqueClipped : OpaqueUnclipped);
	}
	else
	{
		if (BlendMode == BlendMode::RttPremultiplied)
			pGPU->SetRenderState(Clipped ? PremultipliedClipped : PremultipliedUnclipped);
		else
			pGPU->SetRenderState(Clipped ? NormalClipped : NormalUnclipped);
	}
}
//---------------------------------------------------------------------

void CDEMShaderWrapper::prepareForRendering(const ShaderParameterBindings* shaderParameterBindings)
{
	Render::CGPUDriver* pGPU = Renderer.getGPUDriver();

	pGPU->BindConstantBuffer(Render::ShaderType_Vertex, hWMCB, WMCB.Get());
	pGPU->BeginShaderConstants(*WMCB.Get());
	if (hWMCB != hPMCB)
	{
		pGPU->BindConstantBuffer(Render::ShaderType_Vertex, hPMCB, PMCB.Get());
		pGPU->BeginShaderConstants(*PMCB.Get());
	}

	pGPU->BindSampler(Render::ShaderType_Pixel, hLinearSampler, LinearSampler.Get());

	const ShaderParameterBindings::ShaderParameterBindingsMap& paramMap = shaderParameterBindings->getShaderParameterBindings();
	for (auto&& param : paramMap)
	{
		// set param
		// may prepare string -> handle mapping to avoid scanning both shaders each time

		//void CDEMRenderer::setWorldMatrix(const matrix44& Matrix)
		//{
		//	if (WMCB.IsValidPtr())
		//		ConstWorldMatrix->SetRawValue(*WMCB.Get(), reinterpret_cast<const float*>(&Matrix), sizeof(matrix44));
		//}
		////--------------------------------------------------------------------

		//void CDEMRenderer::setProjMatrix(const matrix44& Matrix)
		//{
		//	if (PMCB.IsValidPtr())
		//		ConstProjMatrix->SetRawValue(*PMCB.Get(), reinterpret_cast<const float*>(&Matrix), sizeof(matrix44));
		//}
		////--------------------------------------------------------------------
	}

	if (WMCB.IsValidPtr())
		pGPU->CommitShaderConstants(*WMCB.Get());
	if (hWMCB != hPMCB && PMCB.IsValidPtr())
		pGPU->CommitShaderConstants(*PMCB.Get());
}
//---------------------------------------------------------------------

}
