#pragma once
#ifndef __DEM_L1_RENDER_STATE_DESC_H__
#define __DEM_L1_RENDER_STATE_DESC_H__

#include <Render/RenderFwd.h>
#include <Data/SimpleString.h>
#include <Data/Flags.h>

// API-independent description of a rendering pipeline state. It can be
// serialized, deserialized and used for creation of CRenderState objects.

namespace Render
{

enum ECmpFunc //???to core enums?
{
	Cmp_Never,
	Cmp_Less,
	Cmp_LessEqual,
	Cmp_Greater,
	Cmp_GreaterEqual,
	Cmp_Equal,
	Cmp_NotEqual,
	Cmp_Always
};

enum EStencilOp
{
	StencilOp_Keep,
	StencilOp_Zero,
	StencilOp_Replace,
	StencilOp_Inc,
	StencilOp_IncSat,
	StencilOp_Dec,
	StencilOp_DecSat,
	StencilOp_Invert
};

enum EBlendArg
{
	BlendArg_Zero,
	BlendArg_One,
	BlendArg_SrcColor,
	BlendArg_InvSrcColor,
	BlendArg_Src1Color,
	BlendArg_InvSrc1Color,
	BlendArg_SrcAlpha,
	BlendArg_SrcAlphaSat,
	BlendArg_InvSrcAlpha,
	BlendArg_Src1Alpha,
	BlendArg_InvSrc1Alpha,
	BlendArg_DestColor,
	BlendArg_InvDestColor,
	BlendArg_DestAlpha,
	BlendArg_InvDestAlpha,
	BlendArg_BlendFactor,
	BlendArg_InvBlendFactor
};

enum EBlendOp
{
	BlendOp_Add,
	BlendOp_Sub,
	BlendOp_RevSub,
	BlendOp_Min,
	BlendOp_Max
};

//!!!need const static member with defaults! or method "FillWithDefaults"

struct CRenderStateDesc
{
	// Shaders

	Data::CSimpleString	VertexShaderURI;
	Data::CSimpleString	HullShaderURI;
	Data::CSimpleString	DomainShaderURI;
	Data::CSimpleString	GeometryShaderURI;
	Data::CSimpleString	PixelShaderURI;

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

	int					DepthBias;
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
	struct
	{
		EStencilOp		StencilFailOp;
		EStencilOp		StencilDepthFailOp;
		EStencilOp		StencilPassOp;
		ECmpFunc		StencilFunc;
	}
						StencilFrontFace,
						StencilBackFace;

	// Blend

	enum // For boolean variables
	{
		Blend_AlphaToCoverage		= 0x00010000,
		Blend_Independent			= 0x00020000,	// If not, only RTBlend[0] is used
		Blend_RTBlendEnable			= 0x00040000	// Use (Blend_RTBlendEnable << Index), Index = [0 .. 7]
		// flags from 0x00040000 to 0x02000000 inclusive are reserved for Blend_RTBlendEnable
	};

	struct
	{
		EBlendArg		SrcBlend;
		EBlendArg		DestBlend;
		EBlendOp		BlendOp;
		EBlendArg		SrcBlendAlpha;
		EBlendArg		DestBlendAlpha;
		EBlendOp		BlendOpAlpha;
		unsigned char	WriteMask;
	}
						RTBlend[8];
	float				BlendFactor[4];
	unsigned int		SampleMask;
};

}

#endif
