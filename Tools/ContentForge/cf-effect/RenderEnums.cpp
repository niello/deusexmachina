#include "RenderEnums.h"
#include <Utils.h>

ECmpFunc StringToCmpFunc(std::string Str)
{
	trim(Str, " \r\n\t");
	ToLower(Str);
	if (Str == "less" || Str == "l" || Str == "<") return Cmp_Less;
	if (Str == "lessequal" || Str == "le" || Str == "<=") return Cmp_LessEqual;
	if (Str == "greater" || Str == "g" || Str == ">") return Cmp_Greater;
	if (Str == "greaterequal" || Str == "ge" || Str == ">=") return Cmp_GreaterEqual;
	if (Str == "equal" || Str == "e" || Str == "=" || Str == "==") return Cmp_Equal;
	if (Str == "notequal" || Str == "ne" || Str == "!=") return Cmp_NotEqual;
	if (Str == "always") return Cmp_Always;
	return Cmp_Never;
}
//---------------------------------------------------------------------

EStencilOp StringToStencilOp(std::string Str)
{
	trim(Str, " \r\n\t");
	ToLower(Str);
	if (Str == "zero") return StencilOp_Zero;
	if (Str == "replace") return StencilOp_Replace;
	if (Str == "inc") return StencilOp_Inc;
	if (Str == "incsat") return StencilOp_IncSat;
	if (Str == "dec") return StencilOp_Dec;
	if (Str == "decsat") return StencilOp_DecSat;
	if (Str == "invert") return StencilOp_Invert;
	return StencilOp_Keep;
}
//---------------------------------------------------------------------

EBlendArg StringToBlendArg(std::string Str)
{
	trim(Str, " \r\n\t");
	ToLower(Str);
	if (Str == "one") return BlendArg_One;
	if (Str == "srccolor") return BlendArg_SrcColor;
	if (Str == "invsrccolor") return BlendArg_InvSrcColor;
	if (Str == "src1color") return BlendArg_Src1Color;
	if (Str == "invsrc1color") return BlendArg_InvSrc1Color;
	if (Str == "srcalpha") return BlendArg_SrcAlpha;
	if (Str == "srcalphasat") return BlendArg_SrcAlphaSat;
	if (Str == "invsrcalpha") return BlendArg_InvSrcAlpha;
	if (Str == "src1alpha") return BlendArg_Src1Alpha;
	if (Str == "invsrc1alpha") return BlendArg_InvSrc1Alpha;
	if (Str == "destcolor") return BlendArg_DestColor;
	if (Str == "invdestcolor") return BlendArg_InvDestColor;
	if (Str == "destalpha") return BlendArg_DestAlpha;
	if (Str == "invdestalpha") return BlendArg_InvDestAlpha;
	if (Str == "blendfactor") return BlendArg_BlendFactor;
	if (Str == "invblendfactor") return BlendArg_InvBlendFactor;
	return BlendArg_Zero;
}
//---------------------------------------------------------------------

EBlendOp StringToBlendOp(std::string Str)
{
	trim(Str, " \r\n\t");
	ToLower(Str);
	if (Str == "sub") return BlendOp_Sub;
	if (Str == "revsub") return BlendOp_RevSub;
	if (Str == "min") return BlendOp_Min;
	if (Str == "max") return BlendOp_Max;
	return BlendOp_Add;
}
//---------------------------------------------------------------------

ETexAddressMode StringToTexAddressMode(std::string Str)
{
	trim(Str, " \r\n\t");
	ToLower(Str);
	if (Str == "mirror") return TexAddr_Mirror;
	if (Str == "clamp") return TexAddr_Clamp;
	if (Str == "border") return TexAddr_Border;
	if (Str == "mirroronce") return TexAddr_MirrorOnce;
	return TexAddr_Wrap;
}
//---------------------------------------------------------------------

ETexFilter StringToTexFilter(std::string Str)
{
	trim(Str, " \r\n\t");
	ToLower(Str);
	if (Str == "minmag_point_mip_linear") return TexFilter_MinMag_Point_Mip_Linear;
	if (Str == "min_point_mag_linear_mip_point") return TexFilter_Min_Point_Mag_Linear_Mip_Point;
	if (Str == "min_point_magmip_linear") return TexFilter_Min_Point_MagMip_Linear;
	if (Str == "min_linear_magmip_point") return TexFilter_Min_Linear_MagMip_Point;
	if (Str == "min_linear_mag_point_mip_linear") return TexFilter_Min_Linear_Mag_Point_Mip_Linear;
	if (Str == "minmag_linear_mip_point") return TexFilter_MinMag_Linear_Mip_Point;
	if (Str == "linear" || Str == "minmagmip_linear") return TexFilter_MinMagMip_Linear;
	if (Str == "anisotropic") return TexFilter_Anisotropic;
	return TexFilter_MinMagMip_Point;
}
//---------------------------------------------------------------------
