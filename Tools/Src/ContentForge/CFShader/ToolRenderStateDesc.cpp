#include "ToolRenderStateDesc.h"

namespace Render
{

void CToolRenderStateDesc::SetDefaults()
{
	Flags.ResetTo(Rasterizer_CullBack |
				  Rasterizer_DepthClipEnable |
				  DS_DepthEnable |
				  DS_DepthWriteEnable);

	VertexShader = 0;
	HullShader = 0;
	DomainShader = 0;
	GeometryShader = 0;
	PixelShader = 0;

	DepthBias = 0.f;
	DepthBiasClamp = 0.f;
	SlopeScaledDepthBias = 0.f;

	DepthFunc = Cmp_Less;
	StencilReadMask = 0xff;
	StencilWriteMask = 0xff;
	StencilFrontFace.StencilFunc = Cmp_Always;
	StencilFrontFace.StencilPassOp = StencilOp_Keep;
	StencilFrontFace.StencilFailOp = StencilOp_Keep;
	StencilFrontFace.StencilDepthFailOp = StencilOp_Keep;
	StencilBackFace.StencilFunc = Cmp_Always;
	StencilBackFace.StencilPassOp = StencilOp_Keep;
	StencilBackFace.StencilFailOp = StencilOp_Keep;
	StencilBackFace.StencilDepthFailOp = StencilOp_Keep;
	StencilRef = 0;

	BlendFactorRGBA[0] = 0.f;
	BlendFactorRGBA[1] = 0.f;
	BlendFactorRGBA[2] = 0.f;
	BlendFactorRGBA[3] = 0.f;
	SampleMask = 0xffffffff;

	for (int i = 0; i < 8; ++i)
	{
		CRTBlend& RTB = RTBlend[i];
		RTB.SrcBlendArg = BlendArg_One;
		RTB.DestBlendArg = BlendArg_Zero;
		RTB.BlendOp = BlendOp_Add;
		RTB.SrcBlendArgAlpha = BlendArg_One;
		RTB.DestBlendArgAlpha = BlendArg_Zero;
		RTB.BlendOpAlpha = BlendOp_Add;
		RTB.WriteMask = 0x0f;
	}

	AlphaTestRef = 0;
	AlphaTestFunc = Cmp_Always;
}
//---------------------------------------------------------------------

/*
	RDesc.DepthBias = Desc.Get(CStrID("DepthBias"), 0);
	RDesc.DepthBiasClamp = Desc.Get(CStrID("DepthBiasClamp"), 0.f);
	RDesc.SlopeScaledDepthBias = Desc.Get(CStrID("SlopeScaledDepthBias"), 0.f);
	RDesc.DepthClipEnable = Desc.Get(CStrID("DepthClip"), true);
	RDesc.ScissorEnable = Desc.Get(CStrID("ScissorTest"), true);
	RDesc.MultisampleEnable = Desc.Get(CStrID("Multisampling"), true);
	RDesc.AntialiasedLineEnable = Desc.Get(CStrID("AntialiasedLines"), true);

	IUSMRasterizerState* pRState = NULL;
	if (FAILED(pD3DDevice->CreateRasterizerState(&RDesc, &pRState))) goto ProcessFailure;

	USM_DEPTH_STENCIL_DESC DSDesc;
	DSDesc.DepthEnable = Desc.Get(CStrID("DepthEnable"), true);
	//USM_DEPTH_WRITE_MASK DepthWriteMask;
	//USM_COMPARISON_FUNC DepthFunc;
	DSDesc.StencilEnable = Desc.Get(CStrID("StencilEnable"), true);
	//UINT8 StencilReadMask;
	//UINT8 StencilWriteMask;
	//USM_DEPTH_STENCILOP_DESC FrontFace;
	//USM_DEPTH_STENCILOP_DESC BackFace;

	IUSMDepthStencilState* pDSState = NULL;
	if (FAILED(pD3DDevice->CreateDepthStencilState(&DSDesc, &pDSState))) goto ProcessFailure;

	USM_BLEND_DESC BDesc;
	BDesc.IndependentBlendEnable = Desc.Get(CStrID("IndependentBlendPerTarget"), false);
	BDesc.AlphaToCoverageEnable = Desc.Get(CStrID("AlphaToCoverage"), false);
	if (BDesc.IndependentBlendEnable)
	{
		// CDataArray of sub-descs
		// Init desc, use default values when unspecified
		//!!!AVOID DUPLICATE CODE!
		//BOOL BlendEnable;
		//USM_BLEND SrcBlend;
		//USM_BLEND DestBlend;
		//USM_BLEND_OP BlendOp;
		//USM_BLEND SrcBlendAlpha;
		//USM_BLEND DestBlendAlpha;
		//USM_BLEND_OP BlendOpAlpha;
		//UINT8 RenderTargetWriteMask;
	}
	else
	{
		BDesc.RenderTarget[0].BlendEnable = Desc.Get(CStrID("BlendEnable"), false);
		// Init desc, use default values when unspecified
		//!!!AVOID DUPLICATE CODE!
		//BOOL BlendEnable;
		//USM_BLEND SrcBlend;
		//USM_BLEND DestBlend;
		//USM_BLEND_OP BlendOp;
		//USM_BLEND SrcBlendAlpha;
		//USM_BLEND DestBlendAlpha;
		//USM_BLEND_OP BlendOpAlpha;
		//UINT8 RenderTargetWriteMask;
	}
*/

}