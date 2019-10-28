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
