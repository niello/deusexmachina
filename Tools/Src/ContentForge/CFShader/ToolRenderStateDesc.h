#pragma once
#ifndef __DEM_TOOLS_RENDER_STATE_DESC_H__
#define __DEM_TOOLS_RENDER_STATE_DESC_H__

#include <Render/RenderFwd.h>
#include <Data/Flags.h>

// API-independent description of a rendering pipeline state.
// This is a tool version of desc, using strings instead of resource objects.

namespace Render
{

struct CToolRenderStateDesc
{
	// Shaders

	DWORD				VertexShader;
	DWORD				HullShader;
	DWORD				DomainShader;
	DWORD				GeometryShader;
	DWORD				PixelShader;

	DWORD				InputSignature;

	Data::CFlags		Flags; // For boolean variables, see enums below

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

	float				DepthBias;
	float				DepthBiasClamp;
	float				SlopeScaledDepthBias;

	// Depth-stencil

	enum // For boolean variables
	{
		DS_DepthEnable				= 0x00000100,
		DS_DepthWriteEnable			= 0x00000200,
		DS_StencilEnable			= 0x00000400
	};

	ECmpFunc			DepthFunc;
	unsigned char		StencilReadMask;
	unsigned char		StencilWriteMask;
	struct CStencilSide
	{
		EStencilOp		StencilFailOp;
		EStencilOp		StencilDepthFailOp;
		EStencilOp		StencilPassOp;
		ECmpFunc		StencilFunc;
	}
						StencilFrontFace,
						StencilBackFace;
	unsigned int		StencilRef;

	// Blend

	enum // For boolean variables
	{
		Blend_AlphaToCoverage		= 0x00010000,
		Blend_Independent			= 0x00020000,	// If not, only RTBlend[0] is used
		Blend_RTBlendEnable			= 0x00040000	// Use (Blend_RTBlendEnable << Index), Index = [0 .. 7]
		// flags from				  0x00040000
		//       to					  0x02000000
		// inclusive are reserved for Blend_RTBlendEnable
	};

	struct CRTBlend
	{
		EBlendArg		SrcBlendArg;
		EBlendArg		DestBlendArg;
		EBlendOp		BlendOp;
		EBlendArg		SrcBlendArgAlpha;
		EBlendArg		DestBlendArgAlpha;
		EBlendOp		BlendOpAlpha;
		unsigned char	WriteMask;
	}
						RTBlend[8];
	float				BlendFactorRGBA[4];
	unsigned int		SampleMask;

	// Misc

	enum // For boolean variables
	{
		Misc_AlphaTestEnable		= 0x04000000,
		Misc_ClipPlaneEnable		= 0x08000000 //!!!need 6 bits! or uchar/DWORD w/lower 6 bits
	};
	unsigned char		AlphaTestRef;
	ECmpFunc			AlphaTestFunc;

	void SetDefaults();
};

}

#endif
