#pragma once
#include <Render/RenderEnums.h>
#include <StringID.h>
#include <stdint.h>

// API-independent description of a rendering pipeline state.
// This is a tool version of desc, using strings instead of resource objects.

struct CRenderState
{
	bool IsValid = false;

	// Accumulated info
	// NB: RequiresFlags are D3D11-specific and have no meaning for the effect compiler

	uint32_t ShaderFormatFourCC = 0;
	uint32_t MinFeatureLevel = 0;
	int8_t LightCount = -1;

	// Shaders

	CStrID VertexShader;
	CStrID HullShader;
	CStrID DomainShader;
	CStrID GeometryShader;
	CStrID PixelShader;

	// Rasterizer

	enum // For boolean variables
	{
		Rasterizer_Wireframe		= 0x00000001,
		Rasterizer_FrontCCW			= 0x00000002,
		Rasterizer_CullFront		= 0x00000004,
		Rasterizer_CullBack			= 0x00000008,
		Rasterizer_DepthClipEnable	= 0x00000010,
		Rasterizer_ScissorEnable	= 0x00000020,
		Rasterizer_MSAAEnable		= 0x00000040,
		Rasterizer_MSAALinesEnable	= 0x00000080
	};

	float DepthBias = 0.f;
	float DepthBiasClamp = 0.f;
	float SlopeScaledDepthBias = 0.f;

	// Depth-stencil

	enum // For boolean variables
	{
		DS_DepthEnable				= 0x00000100,
		DS_DepthWriteEnable			= 0x00000200,
		DS_StencilEnable			= 0x00000400
	};

	ECmpFunc			DepthFunc = Cmp_Less;
	unsigned char		StencilReadMask = 0xff;
	unsigned char		StencilWriteMask = 0xff;
	struct CStencilSide
	{
		EStencilOp		StencilFailOp = StencilOp_Keep;
		EStencilOp		StencilDepthFailOp = StencilOp_Keep;
		EStencilOp		StencilPassOp = StencilOp_Keep;
		ECmpFunc		StencilFunc = Cmp_Always;
	};
	CStencilSide		StencilFrontFace;
	CStencilSide		StencilBackFace;
	unsigned int		StencilRef = 0;

	// Blend

	enum // For boolean variables
	{
		Blend_AlphaToCoverage		= 0x00008000,
		Blend_Independent			= 0x00010000,	// If not, only RTBlend[0] is used
		Blend_RTBlendEnable			= 0x00020000	// Use (Blend_RTBlendEnable << Index), Index = [0 .. 7]
		// flags from				  0x00020000
		//       to					  0x01000000
		// inclusive are reserved for Blend_RTBlendEnable, 8 bits total
	};

	struct CRTBlend
	{
		EBlendArg		SrcBlendArg = BlendArg_One;
		EBlendArg		DestBlendArg = BlendArg_Zero;
		EBlendOp		BlendOp = BlendOp_Add;
		EBlendArg		SrcBlendArgAlpha = BlendArg_One;
		EBlendArg		DestBlendArgAlpha = BlendArg_Zero;
		EBlendOp		BlendOpAlpha = BlendOp_Add;
		unsigned char	WriteMask = 0x0f;
	};
	CRTBlend			RTBlend[8];
	float				BlendFactorRGBA[4] = { 0.f };
	unsigned int		SampleMask = 0xffffffff;

	// Misc

	enum // For boolean variables
	{
		Misc_AlphaTestEnable		= 0x02000000,
		Misc_ClipPlaneEnable		= 0x04000000
		// flags from				  0x04000000
		//       to					  0x80000000
		// inclusive are reserved for Misc_ClipPlaneEnable, 6 bits total
	};
	unsigned char		AlphaTestRef = 0;
	ECmpFunc			AlphaTestFunc = Cmp_Always;

	uint32_t Flags = Rasterizer_CullBack | Rasterizer_DepthClipEnable | DS_DepthEnable | DS_DepthWriteEnable;

	void SetFlags(uint32_t Mask, bool On)
	{
		if (On) Flags |= Mask;
		else Flags &= !Mask;
	}
};
