#pragma once
#include <string>

enum EShaderType
{
	ShaderType_Vertex = 0,	// Don't change order and starting index
	ShaderType_Pixel,
	ShaderType_Geometry,
	ShaderType_Hull,
	ShaderType_Domain,

	ShaderType_COUNT,
	ShaderType_Invalid,
	ShaderType_Unknown = ShaderType_Invalid
};

enum EColorMask
{
	ColorMask_Red	= 0x01,
	ColorMask_Green	= 0x02,
	ColorMask_Blue	= 0x04,
	ColorMask_Alpha	= 0x08
};

enum ECmpFunc
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
ECmpFunc StringToCmpFunc(std::string Str);

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
EStencilOp StringToStencilOp(std::string Str);

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
EBlendArg StringToBlendArg(std::string Str);

enum EBlendOp
{
	BlendOp_Add,
	BlendOp_Sub,
	BlendOp_RevSub,
	BlendOp_Min,
	BlendOp_Max
};
EBlendOp StringToBlendOp(std::string Str);

enum ETexAddressMode
{
	TexAddr_Wrap,
	TexAddr_Mirror,
	TexAddr_Clamp,
	TexAddr_Border,
	TexAddr_MirrorOnce
};
ETexAddressMode StringToTexAddressMode(std::string Str);

enum ETexFilter
{
	TexFilter_MinMagMip_Point,
	TexFilter_MinMag_Point_Mip_Linear,
	TexFilter_Min_Point_Mag_Linear_Mip_Point,
	TexFilter_Min_Point_MagMip_Linear,
	TexFilter_Min_Linear_MagMip_Point,
	TexFilter_Min_Linear_Mag_Point_Mip_Linear,
	TexFilter_MinMag_Linear_Mip_Point,
	TexFilter_MinMagMip_Linear,
	TexFilter_Anisotropic
};
ETexFilter StringToTexFilter(std::string Str);

// Don't change order and starting index
enum EPrimitiveTopology
{
	Prim_PointList,
	Prim_LineList,
	Prim_LineStrip,
	Prim_TriList,
	Prim_TriStrip,
	Prim_Invalid
};

// Don't change order and starting index
enum EVertexComponentSemantic
{
	VCSem_Position = 0,
	VCSem_Normal,
	VCSem_Tangent,
	VCSem_Bitangent,
	VCSem_TexCoord,        
	VCSem_Color,
	VCSem_BoneWeights,
	VCSem_BoneIndices,
	VCSem_UserDefined,
	VCSem_Invalid
};

// Don't change order and starting index
enum EVertexComponentFormat
{
	VCFmt_Float32_1,		//> one-component float, expanded to (float, 0, 0, 1)
	VCFmt_Float32_2,		//> two-component float, expanded to (float, float, 0, 1)
	VCFmt_Float32_3,		//> three-component float, expanded to (float, float, float, 1)
	VCFmt_Float32_4,		//> four-component float
	VCFmt_Float16_2,		//> Two 16-bit floating point values, expanded to (value, value, 0, 1)
	VCFmt_Float16_4,		//> Four 16-bit floating point values
	VCFmt_UInt8_4,			//> four-component unsigned byte
	VCFmt_UInt8_4_Norm,		//> four-component normalized unsigned byte (value / 255.0f)
	VCFmt_SInt16_2,			//> two-component signed short, expanded to (value, value, 0, 1)
	VCFmt_SInt16_4,			//> four-component signed short
	VCFmt_SInt16_2_Norm,	//> two-component normalized signed short (value / 32767.0f)
	VCFmt_SInt16_4_Norm,	//> four-component normalized signed short (value / 32767.0f)
	VCFmt_UInt16_2_Norm,	//> two-component normalized unsigned short (value / 65535.0f)
	VCFmt_UInt16_4_Norm,	//> four-component normalized unsigned short (value / 65535.0f)
	VCFmt_Invalid
};
