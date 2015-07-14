#include "RenderStateDesc.h"

namespace Render
{

/*
	RDesc.DepthBias = Desc.Get(CStrID("DepthBias"), 0);
	RDesc.DepthBiasClamp = Desc.Get(CStrID("DepthBiasClamp"), 0.f);
	RDesc.SlopeScaledDepthBias = Desc.Get(CStrID("SlopeScaledDepthBias"), 0.f);
	RDesc.DepthClipEnable = Desc.Get(CStrID("DepthClip"), true);
	RDesc.ScissorEnable = Desc.Get(CStrID("ScissorTest"), true);
	RDesc.MultisampleEnable = Desc.Get(CStrID("Multisampling"), true);
	RDesc.AntialiasedLineEnable = Desc.Get(CStrID("AntialiasedLines"), true);

	ID3D11RasterizerState* pRState = NULL;
	if (FAILED(pD3DDevice->CreateRasterizerState(&RDesc, &pRState))) goto ProcessFailure;

	D3D11_DEPTH_STENCIL_DESC DSDesc;
	DSDesc.DepthEnable = Desc.Get(CStrID("DepthEnable"), true);
	//D3D11_DEPTH_WRITE_MASK DepthWriteMask;
	//D3D11_COMPARISON_FUNC DepthFunc;
	DSDesc.StencilEnable = Desc.Get(CStrID("StencilEnable"), true);
	//UINT8 StencilReadMask;
	//UINT8 StencilWriteMask;
	//D3D11_DEPTH_STENCILOP_DESC FrontFace;
	//D3D11_DEPTH_STENCILOP_DESC BackFace;

	ID3D11DepthStencilState* pDSState = NULL;
	if (FAILED(pD3DDevice->CreateDepthStencilState(&DSDesc, &pDSState))) goto ProcessFailure;

	D3D11_BLEND_DESC BDesc;
	BDesc.IndependentBlendEnable = Desc.Get(CStrID("IndependentBlendPerTarget"), false);
	BDesc.AlphaToCoverageEnable = Desc.Get(CStrID("AlphaToCoverage"), false);
	if (BDesc.IndependentBlendEnable)
	{
		// CDataArray of sub-descs
		// Init desc, use default values when unspecified
		//!!!AVOID DUPLICATE CODE!
		//BOOL BlendEnable;
		//D3D11_BLEND SrcBlend;
		//D3D11_BLEND DestBlend;
		//D3D11_BLEND_OP BlendOp;
		//D3D11_BLEND SrcBlendAlpha;
		//D3D11_BLEND DestBlendAlpha;
		//D3D11_BLEND_OP BlendOpAlpha;
		//UINT8 RenderTargetWriteMask;
	}
	else
	{
		BDesc.RenderTarget[0].BlendEnable = Desc.Get(CStrID("BlendEnable"), false);
		// Init desc, use default values when unspecified
		//!!!AVOID DUPLICATE CODE!
		//BOOL BlendEnable;
		//D3D11_BLEND SrcBlend;
		//D3D11_BLEND DestBlend;
		//D3D11_BLEND_OP BlendOp;
		//D3D11_BLEND SrcBlendAlpha;
		//D3D11_BLEND DestBlendAlpha;
		//D3D11_BLEND_OP BlendOpAlpha;
		//UINT8 RenderTargetWriteMask;
	}
*/

}