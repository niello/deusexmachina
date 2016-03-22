#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/PathUtils.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/Buffer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>
#include <Data/StringTokenizer.h>
#include <Data/StringUtils.h>
#include <Util/UtilFwd.h> // CRC
#include <ToolRenderStateDesc.h>	// Shader references replaced with IDs
#include <Render/SamplerDesc.h>
#include <DEMShaderCompilerDLL.h>
#include <ConsoleApp.h>

#undef CreateDirectory
#undef DeleteFile

extern CString RootPath;

//???maybe pack shaders to some 'DB' file which maps DB ID to offset
//and reference shaders by DB ID, not by a file name?
//can even order shaders in memory and load one-by-one to minimize seek time
//may pack by use (common, menu, game, cinematic etc) and load only used.
//can store debug and release binary packages and read from what user wants
//!!!can pack shaders to a bank here or in BBuilder!

enum EEffectParamType
{
	EPT_SM30Const = 0,
	EPT_SM30Resource,
	EPT_SM30Sampler,
	EPT_SM40Const,
	EPT_SM40Resource,
	EPT_SM40Sampler
};

enum EEffectParamTypeForSaving
{
	EPT_Const		= 0,
	EPT_Resource	= 1,
	EPT_Sampler		= 2,

	EPT_Invalid
};

struct CEffectParam
{
	CStrID				ID;
	EEffectParamType	Type;
	Render::EShaderType	ShaderType;
	U32					SourceShaderID;
	union
	{
		CSM30ShaderConstMeta*	pSM30Const;
		CSM30ShaderRsrcMeta*	pSM30Resource;
		CSM30ShaderSamplerMeta*	pSM30Sampler;
		CD3D11ShaderConstMeta*	pSM40Const;
		CSM40ShaderRsrcMeta*	pSM40Resource;
		CSM40ShaderSamplerMeta*	pSM40Sampler;
	};
	CD3D11ShaderBufferMeta*		pSM40Buffer;	// SM4.0 constants must be in identical buffer, so store for comparison

	bool operator ==(const CEffectParam& Other) const
	{
		if (Type != Other.Type) FAIL;
		if (ShaderType != Other.ShaderType) FAIL;
		switch (Type)
		{
			case EPT_SM30Const:		return *pSM30Const == *Other.pSM30Const;
			case EPT_SM30Resource:	return *pSM30Resource == *Other.pSM30Resource;
			case EPT_SM30Sampler:	return *pSM30Sampler == *Other.pSM30Sampler;
			case EPT_SM40Const:		return (*pSM40Const == *Other.pSM40Const) && (*pSM40Buffer == *Other.pSM40Buffer);
			case EPT_SM40Resource:	return *pSM40Resource == *Other.pSM40Resource;
			case EPT_SM40Sampler:	return *pSM40Sampler == *Other.pSM40Sampler;
		}
		FAIL;
	}

	bool operator !=(const CEffectParam& Other) const { return !(*this == Other); }
};

struct CTechInfo
{
	CStrID						ID;
	CStrID						InputSet;
	UPTR						MaxLights;
	U32							Target;
	U32							MinFeatureLevel;
	U64							RequiresFlags;
	CArray<CStrID>				Passes;
	CFixedArray<UPTR>			PassIndices;
	CFixedArray<bool>			VariationValid;
	CDict<CStrID, CEffectParam>	Params;

	// SM3.0 buffer redefinition
	CArray<UPTR>				UsedFloat4;
	CArray<UPTR>				UsedInt4;
	CArray<UPTR>				UsedBool;
};

struct CRenderStateRef
{
	CStrID							ID;
	UPTR							MaxLights;
	Render::CToolRenderStateDesc	Desc;
	U32								Target;
	U32								MinFeatureLevel;
	U64								RequiresFlags;
	bool							UsesShader[Render::ShaderType_COUNT];
	U32*							ShaderIDs;	// Per shader type, per light count

	CRenderStateRef(): ShaderIDs(NULL) {}
	~CRenderStateRef() { if (ShaderIDs) n_free(ShaderIDs); }

	bool operator ==(const CRenderStateRef& Other) { return ID == Other.ID; }
};

EEffectParamTypeForSaving GetParamTypeForSaving(EEffectParamType Type)
{
	EEffectParamTypeForSaving TypeForSaving;
	switch (Type)
	{
		case EPT_SM30Const:
		case EPT_SM40Const:		TypeForSaving = EPT_Const; break;
		case EPT_SM30Resource:
		case EPT_SM40Resource:	TypeForSaving = EPT_Resource; break;
		case EPT_SM30Sampler:
		case EPT_SM40Sampler:	TypeForSaving = EPT_Sampler; break;
		default:
		{
			n_msg(VL_ERROR, "Unknown tech param type %d\n", (U32)Type);
			return EPT_Invalid;
		}
	};

	return TypeForSaving;
}
//---------------------------------------------------------------------

bool LoadShaderMetadataByObjID(U32 ID,
							   CDict<U32, CSM30ShaderMeta>& D3D9MetaCache, CDict<U32, CD3D11ShaderMeta>& D3D11MetaCache,
							   CSM30ShaderMeta*& pOutD3D9Meta, CD3D11ShaderMeta*& pOutD3D11Meta)
{
	pOutD3D9Meta = NULL;
	pOutD3D11Meta = NULL;

	IPTR Idx = D3D9MetaCache.FindIndex(ID);
	if (Idx != INVALID_INDEX)
	{
		pOutD3D9Meta = &D3D9MetaCache.ValueAt(Idx);
		OK;
	}

	Idx = D3D11MetaCache.FindIndex(ID);
	if (Idx != INVALID_INDEX)
	{
		pOutD3D11Meta = &D3D11MetaCache.ValueAt(Idx);
		OK;
	}

	U32 Target;
	CSM30ShaderMeta* pD3D9Meta;
	CD3D11ShaderMeta* pD3D11Meta;
	if (!DLLLoadShaderMetadataByObjectFileID(ID, Target, pD3D9Meta, pD3D11Meta)) FAIL;

	if (Target >= 0x0400) pOutD3D11Meta = &D3D11MetaCache.Add(ID, *pD3D11Meta);
	else pOutD3D9Meta = &D3D9MetaCache.Add(ID, *pD3D9Meta);

	DLLFreeShaderMetadata(pD3D9Meta, pD3D11Meta);

	FAIL; // Not found in a cache
}
//---------------------------------------------------------------------

void WriteRegisterRanges(const CArray<UPTR>& UsedRegs, IO::CBinaryWriter& W, const char* pRegisterSetName)
{
	U64 RangeCountOffset = W.GetStream().GetPosition();
	W.Write<U32>(0);

	if (!UsedRegs.GetCount()) return;

	U32 RangeCount = 0;
	UPTR CurrStart = UsedRegs[0], CurrCount = 1;
	for (UPTR r = 1; r < UsedRegs.GetCount(); ++r)
	{
		UPTR Reg = UsedRegs[r];
		if (Reg == CurrStart + CurrCount) ++CurrCount;
		else
		{
			// New range detected
			W.Write<U32>(CurrStart);
			W.Write<U32>(CurrCount);
			++RangeCount;
			if (pRegisterSetName) n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
			CurrStart = Reg;
			CurrCount = 1;
		}
	}

	if (CurrStart != (UPTR)-1)
	{
		W.Write<U32>(CurrStart);
		W.Write<U32>(CurrCount);
		++RangeCount;
		if (pRegisterSetName) n_msg(VL_DETAILS, "    Range: %s %d to %d\n", pRegisterSetName, CurrStart, CurrStart + CurrCount - 1);
	}

	U64 EndOffset = W.GetStream().GetPosition();
	W.GetStream().Seek(RangeCountOffset, IO::Seek_Begin);
	W.Write<U32>(RangeCount);
	W.GetStream().Seek(EndOffset, IO::Seek_Begin);
}
//---------------------------------------------------------------------

Render::ECmpFunc StringToCmpFunc(const CString& Str)
{
	if (Str == "less" || Str == "l") return Render::Cmp_Less;
	if (Str == "lessequal" || Str == "le") return Render::Cmp_LessEqual;
	if (Str == "greater" || Str == "g") return Render::Cmp_Greater;
	if (Str == "greaterequal" || Str == "ge") return Render::Cmp_GreaterEqual;
	if (Str == "equal" || Str == "e") return Render::Cmp_Equal;
	if (Str == "notequal" || Str == "ne") return Render::Cmp_NotEqual;
	if (Str == "always") return Render::Cmp_Always;
	return Render::Cmp_Never;
}
//---------------------------------------------------------------------

Render::EStencilOp StringToStencilOp(const CString& Str)
{
	if (Str == "zero") return Render::StencilOp_Zero;
	if (Str == "replace") return Render::StencilOp_Replace;
	if (Str == "inc") return Render::StencilOp_Inc;
	if (Str == "incsat") return Render::StencilOp_IncSat;
	if (Str == "dec") return Render::StencilOp_Dec;
	if (Str == "decsat") return Render::StencilOp_DecSat;
	if (Str == "invert") return Render::StencilOp_Invert;
	return Render::StencilOp_Keep;
}
//---------------------------------------------------------------------

Render::EBlendArg StringToBlendArg(const CString& Str)
{
	if (Str == "one") return Render::BlendArg_One;
	if (Str == "srccolor") return Render::BlendArg_SrcColor;
	if (Str == "invsrccolor") return Render::BlendArg_InvSrcColor;
	if (Str == "src1color") return Render::BlendArg_Src1Color;
	if (Str == "invsrc1color") return Render::BlendArg_InvSrc1Color;
	if (Str == "srcalpha") return Render::BlendArg_SrcAlpha;
	if (Str == "srcalphasat") return Render::BlendArg_SrcAlphaSat;
	if (Str == "invsrcalpha") return Render::BlendArg_InvSrcAlpha;
	if (Str == "src1alpha") return Render::BlendArg_Src1Alpha;
	if (Str == "invsrc1alpha") return Render::BlendArg_InvSrc1Alpha;
	if (Str == "destcolor") return Render::BlendArg_DestColor;
	if (Str == "invdestcolor") return Render::BlendArg_InvDestColor;
	if (Str == "destalpha") return Render::BlendArg_DestAlpha;
	if (Str == "invdestalpha") return Render::BlendArg_InvDestAlpha;
	if (Str == "blendfactor") return Render::BlendArg_BlendFactor;
	if (Str == "invblendfactor") return Render::BlendArg_InvBlendFactor;
	return Render::BlendArg_Zero;
}
//---------------------------------------------------------------------

Render::EBlendOp StringToBlendOp(const CString& Str)
{
	if (Str == "sub") return Render::BlendOp_Sub;
	if (Str == "revsub") return Render::BlendOp_RevSub;
	if (Str == "min") return Render::BlendOp_Min;
	if (Str == "max") return Render::BlendOp_Max;
	return Render::BlendOp_Add;
}
//---------------------------------------------------------------------

Render::ETexAddressMode StringToTexAddressMode(const CString& Str)
{
	if (Str == "mirror") return Render::TexAddr_Mirror;
	if (Str == "clamp") return Render::TexAddr_Clamp;
	if (Str == "border") return Render::TexAddr_Border;
	if (Str == "mirroronce") return Render::TexAddr_MirrorOnce;
	return Render::TexAddr_Wrap;
}
//---------------------------------------------------------------------

Render::ETexFilter StringToTexFilter(const CString& Str)
{
	if (Str == "minmag_point_mip_linear") return Render::TexFilter_MinMag_Point_Mip_Linear;
	if (Str == "min_point_mag_linear_mip_point") return Render::TexFilter_Min_Point_Mag_Linear_Mip_Point;
	if (Str == "min_point_magmip_linear") return Render::TexFilter_Min_Point_MagMip_Linear;
	if (Str == "min_linear_magmip_point") return Render::TexFilter_Min_Linear_MagMip_Point;
	if (Str == "min_linear_mag_point_mip_linear") return Render::TexFilter_Min_Linear_Mag_Point_Mip_Linear;
	if (Str == "minmag_linear_mip_point") return Render::TexFilter_MinMag_Linear_Mip_Point;
	if (Str == "minmagmip_linear") return Render::TexFilter_MinMagMip_Linear;
	if (Str == "anisotropic") return Render::TexFilter_Anisotropic;
	return Render::TexFilter_MinMagMip_Point;
}
//---------------------------------------------------------------------

bool ProcessShaderSection(Data::PParams ShaderSection, Render::EShaderType ShaderType, bool Debug, CRenderStateRef& RSRef)
{
	UPTR LightVariationCount = RSRef.MaxLights + 1;

	// Set invalid shader ID (no shader)
	for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount] = 0;

	RSRef.UsesShader[ShaderType] = ShaderSection.IsValidPtr();
	if (!RSRef.UsesShader[ShaderType]) OK;

	if (Debug) n_msg(VL_DETAILS, "Debug compilation on\n");

	CString SrcPath;
	ShaderSection->Get(SrcPath, CStrID("In"));
	CString EntryPoint;
	ShaderSection->Get(EntryPoint, CStrID("Entry"));
	int Target = 0;
	ShaderSection->Get(Target, CStrID("Target"));
	
	CString Defines;
	if (ShaderSection->Get(Defines, CStrID("Defines"))) Defines.Trim();

	CString WorkingDir;
	Sys::GetWorkingDirectory(WorkingDir);
	SrcPath = IOSrv->ResolveAssigns(SrcPath);
	CString FullSrcPath = PathUtils::GetAbsolutePath(WorkingDir, SrcPath);

	for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
	{
		CString VariationDefines = Defines;
		if (VariationDefines.GetLength()) VariationDefines += ';';
		VariationDefines += "DEM_LIGHT_COUNT=";
		VariationDefines += StringUtils::FromInt(LightCount);

		U32 ObjID, SigID;
		int Result = DLLCompileShader(FullSrcPath.CStr(), (EShaderType)ShaderType, (U32)Target, EntryPoint.CStr(), VariationDefines.CStr(), Debug, ObjID, SigID);

		CString ShortSrcPath(FullSrcPath);
		ShortSrcPath.Replace(IOSrv->ResolveAssigns("SrcShaders:") + "/", "SrcShaders:");

		if (Result == DEM_SHADER_COMPILER_SUCCESS)
		{
			RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount] = ObjID;

			n_msg(VL_DETAILS, "  Shader:   %s -> ID %d\n", ShortSrcPath.CStr(), ObjID);
			if (SigID != 0)
			{
				n_msg(VL_DETAILS, "  InputSig: %s -> ID %d\n", ShortSrcPath.CStr(), SigID);
			}

			const char* pMessages = DLLGetLastOperationMessages();
			if (pMessages) n_msg(VL_WARNING, pMessages);
		}
		else if (Result == DEM_SHADER_COMPILER_COMPILE_ERROR)
		{
			n_msg(VL_ERROR, "Failed to compile '%s' with:\n\n%s\n", ShortSrcPath.CStr(), DLLGetLastOperationMessages());
		}
		else if (Result == DEM_SHADER_COMPILER_REFLECTION_ERROR)
		{
			n_msg(VL_ERROR, "	Shader '%s' reflection error\n", ShortSrcPath.CStr());
		}
		else if (Result == DEM_SHADER_COMPILER_IO_WRITE_ERROR)
		{
			n_msg(VL_ERROR, "	Shader '%s' result saving to HDD failed\n", ShortSrcPath.CStr());
		}
		else if (Result == DEM_SHADER_COMPILER_DB_ERROR)
		{
			n_msg(VL_ERROR, "	Shader '%s' database operation failed\n", ShortSrcPath.CStr());
		}
		else if (Result >= DEM_SHADER_COMPILER_ERROR)
		{
			n_msg(VL_ERROR, "	Shader '%s' compilation failed because of internal error in a DEMShaderCompiler DLL\n", ShortSrcPath.CStr());
			
			const char* pMessages = DLLGetLastOperationMessages();
			if (pMessages) n_msg(VL_ERROR, pMessages);
		}

		/* // Metadata tracing:

		//SM3.0

		n_msg(VL_DETAILS, "    CBuffer: %s\n", Obj.Name.CStr());

		const char* pRegisterSetName = NULL;
		switch (Obj.RegisterSet)
		{
			case RS_Float4:	pRegisterSetName = "float4"; break;
			case RS_Int4:	pRegisterSetName = "int4"; break;
			case RS_Bool:	pRegisterSetName = "bool"; break;
		};
		if (Obj.ElementCount > 1)
		{
			n_msg(VL_DETAILS, "    Const: %s[%d], %s %d to %d\n",
					Obj.Name.CStr(), Obj.ElementCount, pRegisterSetName, Obj.RegisterStart, Obj.RegisterStart + Obj.ElementRegisterCount * Obj.ElementCount - 1);
		}
		else
		{
			n_msg(VL_DETAILS, "    Const: %s, %s %d to %d\n",
					Obj.Name.CStr(), pRegisterSetName, Obj.RegisterStart, Obj.RegisterStart + Obj.ElementRegisterCount * Obj.ElementCount - 1);
		}

		n_msg(VL_DETAILS, "    Texture: %s, slot %d\n", Obj.Name.CStr(), Obj.Register);

		if (Obj.RegisterCount > 1)
		{
			n_msg(VL_DETAILS, "    Sampler: %s[%d], slots %d to %d\n", Obj.Name.CStr(), Obj.RegisterCount, Obj.RegisterStart, Obj.RegisterStart + Obj.RegisterCount - 1);
		}
		else
		{
			n_msg(VL_DETAILS, "    Sampler: %s, slot %d\n", Obj.Name.CStr(), Obj.RegisterStart);
		}

		// SM4.0+

		const char* pBufferTypeStr = "CBuffer";
		const char* pSlotTypeStr = "c";
		UPTR Register = Obj.Register & D3D11Buffer_RegisterMask;
		if (Obj.Register & D3D11Buffer_Texture)
		{
			pBufferTypeStr = "TBuffer";
			pSlotTypeStr = "t";
		}
		else if (Obj.Register & D3D11Buffer_Structured)
		{
			pBufferTypeStr = "SBuffer";
			pSlotTypeStr = "t";
		}
		n_msg(VL_DETAILS, "    %s: %s, %d slot(s) from %s%d\n", pBufferTypeStr, Obj.Name.CStr(), 1, pSlotTypeStr, Register);

		const char* pTypeString = "<unsupported-type>";
		switch (Obj.Type)
		{
			case D3D11Const_Bool:	pTypeString = "bool"; break;
			case D3D11Const_Int:	pTypeString = "int"; break;
			case D3D11Const_Float:	pTypeString = "float"; break;
			case D3D11Const_Struct:	pTypeString = "struct"; break;
		}
		if (Obj.ElementCount > 1)
		{
			n_msg(VL_DETAILS, "      Const: %s %s[%d], offset %d, size %d\n",
					pTypeString, Obj.Name.CStr(), Obj.ElementCount, Obj.Offset, Obj.ElementSize * Obj.ElementCount);
		}
		else
		{
			n_msg(VL_DETAILS, "      Const: %s %s, offset %d, size %d\n",
					pTypeString, Obj.Name.CStr(), Obj.Offset, Obj.ElementSize);
		}
		
		n_msg(VL_DETAILS, "    Texture: %s, %d slot(s) from t%d\n", Obj.Name.CStr(), Obj.RegisterCount, Obj.RegisterStart);
		
		n_msg(VL_DETAILS, "    Sampler: %s, %d slot(s) from s%d\n", Obj.Name.CStr(), Obj.RegisterCount, Obj.RegisterStart);
		*/
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessBlendSection(Data::PParams BlendSection, int Index, Render::CToolRenderStateDesc& Desc)
{
	if (Index < 0 || Index > 7) FAIL;

	CString StrValue;
	bool FlagValue;

	if (BlendSection->Get(FlagValue, CStrID("Enable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Blend_RTBlendEnable << Index, FlagValue);

	Render::CToolRenderStateDesc::CRTBlend& Blend = Desc.RTBlend[Index];

	if (BlendSection->Get(StrValue, CStrID("SrcBlendArg")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.SrcBlendArg = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("SrcBlendArgAlpha")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.SrcBlendArgAlpha = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("DestBlendArg")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.DestBlendArg = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("DestBlendArgAlpha")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.DestBlendArgAlpha = StringToBlendArg(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("BlendOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.BlendOp = StringToBlendOp(StrValue);
	}
	if (BlendSection->Get(StrValue, CStrID("BlendOpAlpha")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Blend.BlendOpAlpha = StringToBlendOp(StrValue);
	}

	if (BlendSection->Get(StrValue, CStrID("WriteMask")))
	{
		unsigned char Mask = 0;
		StrValue.ToLower();
		if (StrValue.FindIndex('r') != INVALID_INDEX) Mask |= Render::ColorMask_Red;
		if (StrValue.FindIndex('g') != INVALID_INDEX) Mask |= Render::ColorMask_Green;
		if (StrValue.FindIndex('b') != INVALID_INDEX) Mask |= Render::ColorMask_Blue;
		if (StrValue.FindIndex('a') != INVALID_INDEX) Mask |= Render::ColorMask_Alpha;
		Blend.WriteMask = Mask;
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessSamplerSection(Data::PParams SamplerSection, Render::CSamplerDesc& Desc)
{
	CString StrValue;
	int IntValue;

	if (SamplerSection->Get(StrValue, CStrID("AddressU")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.AddressU = StringToTexAddressMode(StrValue);
	}
	if (SamplerSection->Get(StrValue, CStrID("AddressV")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.AddressV = StringToTexAddressMode(StrValue);
	}
	if (SamplerSection->Get(StrValue, CStrID("AddressW")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.AddressW = StringToTexAddressMode(StrValue);
	}
	if (SamplerSection->Get(StrValue, CStrID("Filter")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.Filter = StringToTexFilter(StrValue);
	}

	vector4 ColorRGBA;
	if (SamplerSection->Get(ColorRGBA, CStrID("BorderColorRGBA")))
	{
		Desc.BorderColorRGBA[0] = ColorRGBA.v[0];
		Desc.BorderColorRGBA[1] = ColorRGBA.v[1];
		Desc.BorderColorRGBA[2] = ColorRGBA.v[2];
		Desc.BorderColorRGBA[3] = ColorRGBA.v[3];
	}

	SamplerSection->Get(Desc.MipMapLODBias, CStrID("MipMapLODBias"));
	SamplerSection->Get(Desc.FinestMipMapLOD, CStrID("FinestMipMapLOD"));
	SamplerSection->Get(Desc.CoarsestMipMapLOD, CStrID("CoarsestMipMapLOD"));
	if (SamplerSection->Get(IntValue, CStrID("MaxAnisotropy"))) Desc.MaxAnisotropy = IntValue;

	if (SamplerSection->Get(StrValue, CStrID("CmpFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.CmpFunc = StringToCmpFunc(StrValue);
	}

	OK;
}
//---------------------------------------------------------------------

bool ReadRenderStateDesc(Data::PParams RenderStates, CStrID ID, Render::CToolRenderStateDesc& Desc, Data::PParams* ShaderSections)
{
	Data::PParams RS;
	if (!RenderStates->Get(RS, ID)) FAIL;

	//!!!may store already loaded in cache to avoid unnecessary reloading!
	Data::CParam* pPrmBaseID;
	if (RS->Get(pPrmBaseID, CStrID("Base")))
	{
		if (!ReadRenderStateDesc(RenderStates, pPrmBaseID->GetValue<CStrID>(), Desc, ShaderSections)) FAIL;
	}

	// Shaders

	const char* pShaderSectionName[] = { "VS", "PS", "GS", "HS", "DS" };

	Data::PParams ShaderSection;
	for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
	{
		if (RS->Get(ShaderSection, CStrID(pShaderSectionName[ShaderType])))
		{
			if (!ShaderSections) return ERR_MAIN_FAILED;
			Data::PParams CurrShaderSection = ShaderSections[ShaderType];
			if (CurrShaderSection.IsValidPtr() && CurrShaderSection->GetCount())
				CurrShaderSection->Merge(*ShaderSection, Data::Merge_AddNew | Data::Merge_Replace | Data::Merge_Deep);
			else
				ShaderSections[ShaderType] = ShaderSection;
		}
	}

	// States

	CString StrValue;
	int IntValue;
	bool FlagValue;
	vector4 Vector4Value;

	// Rasterizer

	if (RS->Get(StrValue, CStrID("Cull")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		if (StrValue == "none")
		{
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullFront);
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullBack);
		}
		else if (StrValue == "front")
		{
			Desc.Flags.Set(Render::CToolRenderStateDesc::Rasterizer_CullFront);
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullBack);
		}
		else if (StrValue == "back")
		{
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Rasterizer_CullFront);
			Desc.Flags.Set(Render::CToolRenderStateDesc::Rasterizer_CullBack);
		}
		else
		{
			n_msg(VL_ERROR, "Unrecognized 'Cull' value: %s\n", StrValue.CStr());
			FAIL;
		}
	}

	if (RS->Get(FlagValue, CStrID("Wireframe")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_Wireframe, FlagValue);
	if (RS->Get(FlagValue, CStrID("FrontCCW")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_FrontCCW, FlagValue);
	if (RS->Get(FlagValue, CStrID("DepthClipEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_DepthClipEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("ScissorEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_ScissorEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("MSAAEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_MSAAEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("MSAALinesEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Rasterizer_MSAALinesEnable, FlagValue);

	RS->Get(Desc.DepthBias, CStrID("DepthBias"));
	RS->Get(Desc.DepthBiasClamp, CStrID("DepthBiasClamp"));
	RS->Get(Desc.SlopeScaledDepthBias, CStrID("SlopeScaledDepthBias"));

	// Depth-stencil

	if (RS->Get(FlagValue, CStrID("DepthEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::DS_DepthEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("DepthWriteEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::DS_DepthWriteEnable, FlagValue);
	if (RS->Get(FlagValue, CStrID("StencilEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::DS_StencilEnable, FlagValue);

	if (RS->Get(StrValue, CStrID("DepthFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.DepthFunc = StringToCmpFunc(StrValue);
	}
	if (RS->Get(IntValue, CStrID("StencilReadMask"))) Desc.StencilReadMask = IntValue;
	if (RS->Get(IntValue, CStrID("StencilWriteMask"))) Desc.StencilWriteMask = IntValue;
	if (RS->Get(StrValue, CStrID("StencilFrontFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilFunc = StringToCmpFunc(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilFrontPassOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilPassOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilFrontFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilFrontDepthFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilFrontFace.StencilDepthFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilFunc = StringToCmpFunc(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackPassOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilPassOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(StrValue, CStrID("StencilBackDepthFailOp")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.StencilBackFace.StencilDepthFailOp = StringToStencilOp(StrValue);
	}
	if (RS->Get(IntValue, CStrID("StencilRef"))) Desc.StencilRef = IntValue;

	// Blend

	if (RS->Get(FlagValue, CStrID("AlphaToCoverage")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Blend_AlphaToCoverage, FlagValue);

	if (RS->Get(Vector4Value, CStrID("BlendFactor")))
		memcpy(Desc.BlendFactorRGBA, Vector4Value.v, sizeof(float) * 4);
	if (RS->Get(IntValue, CStrID("SampleMask"))) Desc.SampleMask = IntValue;

	Data::CParam* pBlend;
	if (RS->Get(pBlend, CStrID("Blend")))
	{
		if (pBlend->IsA<Data::PParams>())
		{
			Data::PParams BlendSection = pBlend->GetValue<Data::PParams>();
			Desc.Flags.Clear(Render::CToolRenderStateDesc::Blend_Independent);
			if (!ProcessBlendSection(BlendSection, 0, Desc)) FAIL;
		}
		else if (pBlend->IsA<Data::PDataArray>())
		{
			Data::PDataArray BlendArray = pBlend->GetValue<Data::PDataArray>();
			Desc.Flags.Set(Render::CToolRenderStateDesc::Blend_Independent);
			for (UPTR BlendIdx = 0; BlendIdx < BlendArray->GetCount(); ++BlendIdx)
			{
				Data::PParams BlendSection = BlendArray->Get<Data::PParams>(BlendIdx);
				if (!ProcessBlendSection(BlendSection, BlendIdx, Desc)) FAIL;
			}
		}
		else FAIL;
	}

	// Misc

	if (RS->Get(FlagValue, CStrID("AlphaTestEnable")))
		Desc.Flags.SetTo(Render::CToolRenderStateDesc::Misc_AlphaTestEnable, FlagValue);

	if (RS->Get(IntValue, CStrID("AlphaTestRef"))) Desc.AlphaTestRef = IntValue;
	if (RS->Get(StrValue, CStrID("AlphaTestFunc")))
	{
		StrValue.Trim();
		StrValue.ToLower();
		Desc.AlphaTestFunc = StringToCmpFunc(StrValue);
	}

	Data::CData ClipPlanes;
	if (RS->Get(ClipPlanes, CStrID("ClipPlanes")))
	{
		if (ClipPlanes.IsA<bool>() && ClipPlanes == false)
		{
			// All clip planes disabled
		}
		else if (ClipPlanes.IsA<int>())
		{
			int CP = ClipPlanes;
			for (int i = 0; i < 5; ++i)
				Desc.Flags.SetTo(Render::CToolRenderStateDesc::Misc_ClipPlaneEnable << i, !!(CP & (1 << i)));
		}
		else if (ClipPlanes.IsA<Data::PDataArray>())
		{
			Data::CDataArray& CP = *ClipPlanes.GetValue<Data::PDataArray>();
			
			for (UPTR i = 0; i < 5; ++i)
				Desc.Flags.Clear(Render::CToolRenderStateDesc::Misc_ClipPlaneEnable << i);
			
			for (UPTR i = 0; i < CP.GetCount(); ++i)
			{
				Data::CData& Val = CP[i];
				if (!Val.IsA<int>()) continue;
				int IntVal = Val;
				if (IntVal < 0 || IntVal > 5) continue;
				Desc.Flags.Set(Render::CToolRenderStateDesc::Misc_ClipPlaneEnable << IntVal);
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

int CompileEffect(const char* pInFilePath, const char* pOutFilePath, bool Debug)
{
	// Read effect source file

	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(pInFilePath, Buffer)) return ERR_IO_READ;

	Data::PParams Params;
	{
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), Params)) return ERR_IO_READ;
	}

	Data::PParams Techs;
	Data::PParams RenderStates;
	if (!Params->Get(Techs, CStrID("Techniques"))) return ERR_INVALID_DATA;
	if (!Params->Get(RenderStates, CStrID("RenderStates"))) return ERR_INVALID_DATA;

	// Collect techs and render states they use

	CArray<CTechInfo> UsedTechs;
	CDict<CStrID, CRenderStateRef> UsedRenderStates;
	for (UPTR TechIdx = 0; TechIdx < Techs->GetCount(); ++TechIdx)
	{
		Data::CParam& Tech = Techs->Get(TechIdx);
		Data::PParams TechDesc = Tech.GetValue<Data::PParams>();

		Data::PDataArray Passes;
		if (!TechDesc->Get(Passes, CStrID("Passes"))) continue;
		CStrID InputSet = TechDesc->Get(CStrID("InputSet"), CStrID::Empty);
		if (!InputSet.IsValid()) continue;
		UPTR MaxLights = (UPTR)n_max(TechDesc->Get<int>(CStrID("MaxLights"), 0), 0);
		UPTR LightVariationCount = MaxLights + 1;

		CTechInfo* pTechInfo = UsedTechs.Add();
		pTechInfo->ID = Tech.GetName();
		pTechInfo->InputSet = InputSet;
		pTechInfo->MaxLights = MaxLights;

		for (UPTR PassIdx = 0; PassIdx < Passes->GetCount(); ++PassIdx)
		{
			CStrID PassID = Passes->Get<CStrID>(PassIdx);
			pTechInfo->Passes.Add(PassID);
			n_msg(VL_DETAILS, "Tech: %s, Pass %d: %s, MaxLights: %d\n", Tech.GetName().CStr(), PassIdx, PassID.CStr(), MaxLights);
			IPTR Idx = UsedRenderStates.FindIndex(PassID);
			if (Idx == INVALID_INDEX)
			{
				CRenderStateRef& NewPass = UsedRenderStates.Add(PassID);
				NewPass.ID = PassID;
				NewPass.MaxLights = MaxLights;
				NewPass.ShaderIDs = (U32*)n_malloc(Render::ShaderType_COUNT * LightVariationCount * sizeof(U32));
			}
			else
			{
				CRenderStateRef& Pass = UsedRenderStates.ValueAt(Idx);
				if (MaxLights > Pass.MaxLights)
				{
					Pass.MaxLights = MaxLights;
					Pass.ShaderIDs = (U32*)n_realloc(Pass.ShaderIDs, Render::ShaderType_COUNT * LightVariationCount * sizeof(U32));
				}
			}
		}
	}

	// Compile and validate used render states, unwinding their hierarchy

	CDict<U32, CSM30ShaderMeta> D3D9MetaCache;
	CDict<U32, CD3D11ShaderMeta> D3D11MetaCache;

	for (UPTR i = 0; i < UsedRenderStates.GetCount(); )
	{
		Data::PParams ShaderSections[Render::ShaderType_COUNT];

		// Load states only, collect shader sections
		CRenderStateRef& RSRef = UsedRenderStates.ValueAt(i);
		RSRef.Desc.SetDefaults();
		if (!ReadRenderStateDesc(RenderStates, RSRef.ID, RSRef.Desc, ShaderSections))
		{
			// Loading failed, discard this render state
			n_msg(VL_WARNING, "Render state '%s' parsing failed\n", UsedRenderStates.KeyAt(i).CStr());
			UsedRenderStates.RemoveAt(i);
			continue;
		}

		// Validate shader combination and collect its requirements

		RSRef.Target = 0x0000;
		RSRef.MinFeatureLevel = 0;
		RSRef.RequiresFlags = 0;
		
		bool Failed = false;
		
		for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
		{
			Data::PParams ShaderSection = ShaderSections[ShaderType];
			if (ShaderSection.IsNullPtr()) continue;
			
			int Target = 0;
			if (!ShaderSection->Get(Target, CStrID("Target"))) continue;
			if (!RSRef.Target) RSRef.Target = (U32)Target;
			else if ((RSRef.Target < 0x0400 && Target >= 0x0400) || (RSRef.Target >= 0x0400 && Target < 0x0400))
			{
				Failed = true;
				break;
			}
			else if ((U32)Target > RSRef.Target) RSRef.Target = (U32)Target;
		}
		
		if (Failed)
		{
			n_msg(VL_ERROR, "Render state '%s' mixes sm3.0 and sm4.0+ shaders, which is invalid\n", RSRef.ID.CStr());
			UsedRenderStates.RemoveAt(i);
			continue;
		}

		UPTR LightVariationCount = RSRef.MaxLights + 1;

		// Compile shaders from collected section for each light count variation
		for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
		{
			ProcessShaderSection(ShaderSections[ShaderType], (Render::EShaderType)ShaderType, Debug, RSRef);

			// If some of used shaders failed to load in all variations, discard this render state
			if (RSRef.UsesShader[ShaderType])
			{
				bool AllInvalid = true;;
				for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
				{
					U32 ShaderID = RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount];
					if (ShaderID != 0)
					{
						// Valid shader found, get its requirements and apply to render state requirements
						AllInvalid = false;

						CSM30ShaderMeta* pD3D9Meta = NULL;
						CD3D11ShaderMeta* pD3D11Meta = NULL;
						LoadShaderMetadataByObjID(ShaderID, D3D9MetaCache, D3D11MetaCache, pD3D9Meta, pD3D11Meta);

						if (pD3D9Meta)
						{
							RSRef.MinFeatureLevel = n_max(RSRef.MinFeatureLevel, Render::GPU_Level_D3D9_3);
						}
						else if (pD3D11Meta)
						{
							RSRef.MinFeatureLevel = n_max(RSRef.MinFeatureLevel, pD3D11Meta->MinFeatureLevel);
							RSRef.RequiresFlags |= pD3D11Meta->RequiresFlags;
						}
						else Sys::Error("FIXME: Can't read shader metadata!");
					}
				}

				if (AllInvalid)
				{
					const char* pShaderNames[] = { "vertex", "pixel", "geometry", "hull", "domain" };
					n_msg(VL_WARNING, "Render state '%s' %s shader compilation failed for all variations\n", UsedRenderStates.KeyAt(i).CStr(), pShaderNames[ShaderType]);
					Failed = true;
					break;
				}
			}
		}

		if (Failed)
		{
			UsedRenderStates.RemoveAt(i);
			continue;
		}

		++i;
	}

	// Resolve pass refs in techs, discard light count variations and whole techs where
	// at least one render state failed to compile.
	// Reduce MaxLights if all variations above a value are invalid.
	// Validate that all tech passes are targeting the same API.

	for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount();)
	{
		CTechInfo& TechInfo = UsedTechs[TechIdx];

		UPTR LightVariationCount = TechInfo.MaxLights + 1;
		UPTR LastValidVariation = INVALID_INDEX;

		TechInfo.PassIndices.SetSize(TechInfo.Passes.GetCount());
		TechInfo.VariationValid.SetSize(LightVariationCount);
		TechInfo.Target = 0x0000;
		TechInfo.MinFeatureLevel = 0;
		TechInfo.RequiresFlags = 0;

		bool DiscardTech = false;

		// Cache pass render state indices, discard tech if any pass was discarded
		for (UPTR PassIdx = 0; PassIdx < TechInfo.Passes.GetCount(); ++PassIdx)
		{
			IPTR Idx = UsedRenderStates.FindIndex(TechInfo.Passes[PassIdx]);
			if (Idx == INVALID_INDEX)
			{
				DiscardTech = true;
				break;
			}
			else TechInfo.PassIndices[PassIdx] = (UPTR)Idx;

			CRenderStateRef& RSRef = UsedRenderStates.ValueAt(Idx);
			
			U32 PassTarget = RSRef.Target;
			if (!TechInfo.Target) TechInfo.Target = PassTarget;
			else if ((TechInfo.Target < 0x0400 && PassTarget >= 0x0400) || (TechInfo.Target >= 0x0400 && PassTarget < 0x0400))
			{
				n_msg(VL_ERROR, "Tech %s mixes sm3.0 and sm4.0+ passes, which is invalid\n", TechInfo.ID.CStr());
				DiscardTech = true;
				break;
			}
			else if (PassTarget > TechInfo.Target) TechInfo.Target = PassTarget;

			TechInfo.MinFeatureLevel = n_max(TechInfo.MinFeatureLevel, RSRef.MinFeatureLevel);
			TechInfo.RequiresFlags |= RSRef.RequiresFlags;
		}

		if (DiscardTech)
		{
			n_msg(VL_WARNING, "Tech '%s' discarded due to discarded passes\n", TechInfo.ID.CStr());
			UsedTechs.RemoveAt(TechIdx);
			continue;
		}

		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		{
			TechInfo.VariationValid[LightCount] = true;

			// If variation uses shader that failed to compile, discard variation
			for (UPTR PassIdx = 0; PassIdx < TechInfo.Passes.GetCount(); ++PassIdx)
			{
				CRenderStateRef& RSRef = UsedRenderStates.ValueAt(TechInfo.PassIndices[PassIdx]);
				for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
				{
					if (RSRef.UsesShader[ShaderType] && RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount] == 0)
					{
						TechInfo.VariationValid[LightCount] = false;
						break;
					}
				}
			}

			if (TechInfo.VariationValid[LightCount]) LastValidVariation = LightCount;
		}

		// Discard tech without valid variations
		if (LastValidVariation == INVALID_INDEX)
		{
			n_msg(VL_WARNING, "Tech '%s' discarded as it has no valid variations\n", TechInfo.ID.CStr());
			UsedTechs.RemoveAt(TechIdx);
			continue;
		}

		TechInfo.MaxLights = LastValidVariation;

		++TechIdx;
	}

	// Discard an effect if there are no valid techs

	if (!UsedTechs.GetCount())
	{
		n_msg(VL_WARNING, "Effect '%s' is not compiled, because it has no valid techs\n", pInFilePath);
		return ERR_INVALID_DATA;
	}

	// Collect and validate tech params

	//!!!NB: if the same param is used in different stages and in different CBs, setting it
	//in a tech requires passing CB instance per stage! May be restrict to use one param only
	//in the same CB in all stages or even use one param only in one stage instead.
	// Each shader stage that param uses may define a param differently

	for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
	{
		CTechInfo& TechInfo = UsedTechs[TechIdx];
		UPTR LightVariationCount = TechInfo.MaxLights + 1;

		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
		{
			for (UPTR PassIdx = 0; PassIdx < TechInfo.Passes.GetCount(); ++PassIdx)
			{
				CRenderStateRef& RSRef = UsedRenderStates.ValueAt(TechInfo.PassIndices[PassIdx]);
				for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
				{
					if (!RSRef.UsesShader[ShaderType]) continue;

					U32 ShaderID = RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount];
					CSM30ShaderMeta* pD3D9Meta = NULL;
					CD3D11ShaderMeta* pD3D11Meta = NULL;
					LoadShaderMetadataByObjID(ShaderID, D3D9MetaCache, D3D11MetaCache, pD3D9Meta, pD3D11Meta);

					//!!!add per-stage support, to map one param to different shader stages simultaneously!
					// If tech param found:
					// Warn if there are more than one stage that parameter uses (warn once, when we add second stage to a param desc)
					// If not registered for this stage, add per-stage metadata
					// If only the same metadata allowed to different stages, compare with any reference (processed) stage
					// In this case code remains almost unchanged, but param instead of single shader stage stores stage mask.
					if (pD3D9Meta)
					{
						for (UPTR ParamIdx = 0; ParamIdx < pD3D9Meta->Consts.GetCount(); ++ParamIdx)
						{
							CSM30ShaderConstMeta& MetaObj = pD3D9Meta->Consts[ParamIdx];
							CStrID MetaObjID = CStrID(MetaObj.Name.CStr());
							
							IPTR Idx = TechInfo.Params.FindIndex(MetaObjID);
							if (Idx == INVALID_INDEX)
							{
								CEffectParam& Param = TechInfo.Params.Add(MetaObjID);
								Param.ID = MetaObjID;
								Param.Type = EPT_SM30Const;
								Param.ShaderType = (Render::EShaderType)ShaderType;
								Param.SourceShaderID = ShaderID;
								Param.pSM30Const = &MetaObj;
								n_msg(VL_DEBUG, "Tech '%s': param '%s' (const) added\n", TechInfo.ID.CStr(), MetaObjID.CStr());
							}
							else
							{
								const CEffectParam& Param = TechInfo.Params.ValueAt(Idx);
								if (Param.Type != EPT_SM30Const || !Param.pSM30Const)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different class in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							
								CSM30ShaderConstMeta& RefMetaObj = *Param.pSM30Const;
								if (MetaObj != RefMetaObj)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							}
						}

						for (UPTR ParamIdx = 0; ParamIdx < pD3D9Meta->Resources.GetCount(); ++ParamIdx)
						{
							CSM30ShaderRsrcMeta& MetaObj = pD3D9Meta->Resources[ParamIdx];
							CStrID MetaObjID = CStrID(MetaObj.Name.CStr());
							
							IPTR Idx = TechInfo.Params.FindIndex(MetaObjID);
							if (Idx == INVALID_INDEX)
							{
								CEffectParam& Param = TechInfo.Params.Add(MetaObjID);
								Param.ID = MetaObjID;
								Param.Type = EPT_SM30Resource;
								Param.ShaderType = (Render::EShaderType)ShaderType;
								Param.SourceShaderID = ShaderID;
								Param.pSM30Resource = &MetaObj;
								n_msg(VL_DEBUG, "Tech '%s': param '%s' (resource) added\n", TechInfo.ID.CStr(), MetaObjID.CStr());
							}
							else
							{
								const CEffectParam& Param = TechInfo.Params.ValueAt(Idx);
								if (Param.Type != EPT_SM30Resource || !Param.pSM30Resource)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different class in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							
								CSM30ShaderRsrcMeta& RefMetaObj = *Param.pSM30Resource;
								if (MetaObj != RefMetaObj)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							}
						}

						for (UPTR ParamIdx = 0; ParamIdx < pD3D9Meta->Samplers.GetCount(); ++ParamIdx)
						{
							CSM30ShaderSamplerMeta& MetaObj = pD3D9Meta->Samplers[ParamIdx];
							CStrID MetaObjID = CStrID(MetaObj.Name.CStr());
							
							IPTR Idx = TechInfo.Params.FindIndex(MetaObjID);
							if (Idx == INVALID_INDEX)
							{
								CEffectParam& Param = TechInfo.Params.Add(MetaObjID);
								Param.ID = MetaObjID;
								Param.Type = EPT_SM30Sampler;
								Param.ShaderType = (Render::EShaderType)ShaderType;
								Param.SourceShaderID = ShaderID;
								Param.pSM30Sampler = &MetaObj;
								n_msg(VL_DEBUG, "Tech '%s': param '%s' (sampler) added\n", TechInfo.ID.CStr(), MetaObjID.CStr());
							}
							else
							{
								const CEffectParam& Param = TechInfo.Params.ValueAt(Idx);
								if (Param.Type != EPT_SM30Sampler || !Param.pSM30Sampler)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different class in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							
								CSM30ShaderSamplerMeta& RefMetaObj = *Param.pSM30Sampler;
								if (MetaObj != RefMetaObj)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							}
						}
					}
					else if (pD3D11Meta)
					{
						for (UPTR ParamIdx = 0; ParamIdx < pD3D11Meta->Consts.GetCount(); ++ParamIdx)
						{
							CD3D11ShaderConstMeta& MetaObj = pD3D11Meta->Consts[ParamIdx];
							CD3D11ShaderBufferMeta& MetaBuf = pD3D11Meta->Buffers[MetaObj.BufferIndex];
							CStrID MetaObjID = CStrID(MetaObj.Name.CStr());
							
							IPTR Idx = TechInfo.Params.FindIndex(MetaObjID);
							if (Idx == INVALID_INDEX)
							{
								CEffectParam& Param = TechInfo.Params.Add(MetaObjID);
								Param.ID = MetaObjID;
								Param.Type = EPT_SM40Const;
								Param.ShaderType = (Render::EShaderType)ShaderType;
								Param.SourceShaderID = ShaderID;
								Param.pSM40Const = &MetaObj;
								Param.pSM40Buffer = &MetaBuf;
								n_msg(VL_DEBUG, "Tech '%s': param '%s' (const) added\n", TechInfo.ID.CStr(), MetaObjID.CStr());
							}
							else
							{
								const CEffectParam& Param = TechInfo.Params.ValueAt(Idx);
								if (Param.Type != EPT_SM40Const || !Param.pSM40Const)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different class in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							
								CD3D11ShaderConstMeta& RefMetaObj = *Param.pSM40Const;
								if (MetaObj != RefMetaObj)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}

								CD3D11ShaderBufferMeta& RefMetaBuf = *Param.pSM40Buffer;
								if (MetaBuf != RefMetaBuf)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' containing buffers have different description in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							}
						}
						
						for (UPTR ParamIdx = 0; ParamIdx < pD3D11Meta->Resources.GetCount(); ++ParamIdx)
						{
							CSM40ShaderRsrcMeta& MetaObj = pD3D11Meta->Resources[ParamIdx];
							CStrID MetaObjID = CStrID(MetaObj.Name.CStr());
							
							IPTR Idx = TechInfo.Params.FindIndex(MetaObjID);
							if (Idx == INVALID_INDEX)
							{
								CEffectParam& Param = TechInfo.Params.Add(MetaObjID);
								Param.ID = MetaObjID;
								Param.Type = EPT_SM40Resource;
								Param.ShaderType = (Render::EShaderType)ShaderType;
								Param.SourceShaderID = ShaderID;
								Param.pSM40Resource = &MetaObj;
								n_msg(VL_DEBUG, "Tech '%s': param '%s' (resource) added\n", TechInfo.ID.CStr(), MetaObjID.CStr());
							}
							else
							{
								const CEffectParam& Param = TechInfo.Params.ValueAt(Idx);
								if (Param.Type != EPT_SM40Resource || !Param.pSM40Resource)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different class in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							
								CSM40ShaderRsrcMeta& RefMetaObj = *Param.pSM40Resource;
								if (MetaObj != RefMetaObj)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							}
						}
						
						for (UPTR ParamIdx = 0; ParamIdx < pD3D11Meta->Samplers.GetCount(); ++ParamIdx)
						{
							CSM40ShaderSamplerMeta& MetaObj = pD3D11Meta->Samplers[ParamIdx];
							CStrID MetaObjID = CStrID(MetaObj.Name.CStr());
							
							IPTR Idx = TechInfo.Params.FindIndex(MetaObjID);
							if (Idx == INVALID_INDEX)
							{
								CEffectParam& Param = TechInfo.Params.Add(MetaObjID);
								Param.ID = MetaObjID;
								Param.Type = EPT_SM40Sampler;
								Param.ShaderType = (Render::EShaderType)ShaderType;
								Param.SourceShaderID = ShaderID;
								Param.pSM40Sampler = &MetaObj;
								n_msg(VL_DEBUG, "Tech '%s': param '%s' (sampler) added\n", TechInfo.ID.CStr(), MetaObjID.CStr());
							}
							else
							{
								const CEffectParam& Param = TechInfo.Params.ValueAt(Idx);
								if (Param.Type != EPT_SM40Sampler || !Param.pSM40Sampler)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different class in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							
								CSM40ShaderSamplerMeta& RefMetaObj = *Param.pSM40Sampler;
								if (MetaObj != RefMetaObj)
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjID.CStr());
									return ERR_INVALID_DATA;
								}
							}
						}
					}
					else
					{
						n_msg(VL_ERROR, "Failed to load shader metadata for obj ID %d\n", ShaderID);
						return ERR_INVALID_DATA;
					}
				}
			}
		}
	}

	// Build global and material param tables

	CArray<CEffectParam> GlobalParams;
	CArray<CEffectParam> MaterialParams;

	// SM4.0+
	CArray<U32> GlobalRegisters;
	CArray<U32> MaterialRegisters;

	// SM3.0
	CArray<UPTR> GlobalFloat4;
	CArray<UPTR> GlobalInt4;
	CArray<UPTR> GlobalBool;
	CArray<UPTR> MaterialFloat4;
	CArray<UPTR> MaterialInt4;
	CArray<UPTR> MaterialBool;

	Data::PParams GlobalParamsDesc;
	if (Params->Get(GlobalParamsDesc, CStrID("GlobalParams")))
	{
		for (UPTR ParamIdx = 0; ParamIdx < GlobalParamsDesc->GetCount(); ++ParamIdx)
		{
			Data::CParam& ShaderParam = GlobalParamsDesc->Get(ParamIdx);
			CStrID ParamID = ShaderParam.GetName();

			IPTR GlobalParamIdx = INVALID_INDEX;
			for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
			{
				CTechInfo& TechInfo = UsedTechs[TechIdx];

				IPTR Idx = TechInfo.Params.FindIndex(ParamID);
				if (Idx == INVALID_INDEX) continue;

				CEffectParam& TechParam = TechInfo.Params.ValueAt(Idx);

				if (GlobalParamIdx == INVALID_INDEX)
				{
					GlobalParamIdx = GlobalParams.IndexOf(GlobalParams.Add(TechParam));
					n_msg(VL_DEBUG, "Global param '%s' added\n", ParamID.CStr());

					if (TechParam.Type == EPT_SM40Const)
					{
						U32 BufferRegister = TechParam.pSM40Buffer->Register;
						if (MaterialRegisters.Contains(BufferRegister))
						{
							n_msg(VL_ERROR, "Global param '%s' is placed in a buffer with material params\n", ParamID.CStr());
							return ERR_INVALID_DATA;
						}
						if (!GlobalRegisters.Contains(BufferRegister)) GlobalRegisters.Add(BufferRegister);
					}
					else if (TechParam.Type == EPT_SM30Const)
					{
						CArray<UPTR>& UsedGlobalRegs = (TechParam.pSM30Const->RegisterSet == RS_Float4) ? GlobalFloat4 : ((TechParam.pSM30Const->RegisterSet == RS_Int4) ? GlobalInt4 : GlobalBool);
						CArray<UPTR>& UsedMaterialRegs = (TechParam.pSM30Const->RegisterSet == RS_Float4) ? MaterialFloat4 : ((TechParam.pSM30Const->RegisterSet == RS_Int4) ? MaterialInt4 : MaterialBool);
						for (UPTR r = TechParam.pSM30Const->RegisterStart; r < TechParam.pSM30Const->RegisterStart + TechParam.pSM30Const->RegisterCount; ++r)
						{
							if (UsedMaterialRegs.Contains(r))
							{
								n_msg(VL_ERROR, "Global param '%s' uses a register used by material params\n", ParamID.CStr());
								return ERR_INVALID_DATA;
							}
							if (!UsedGlobalRegs.Contains(r)) UsedGlobalRegs.Add(r);
						}
					}
				}
				else
				{
					CEffectParam& MaterialParam = GlobalParams[GlobalParamIdx];
					if (TechParam != MaterialParam)
					{
						n_msg(VL_ERROR, "Global param '%s' is defined differently in different techs, which is invalid\n", ParamID.CStr());
						return ERR_INVALID_DATA;
					}
				}
			}

			if (GlobalParamIdx == INVALID_INDEX)
			{
				n_msg(VL_WARNING, "Global param '%s' is not used in any valid tech\n", ParamID.CStr());
			}
		}
	}

	Data::PParams MaterialParamsDesc;
	if (Params->Get(MaterialParamsDesc, CStrID("MaterialParams")))
	{
		for (UPTR ParamIdx = 0; ParamIdx < MaterialParamsDesc->GetCount(); ++ParamIdx)
		{
			Data::CParam& ShaderParam = MaterialParamsDesc->Get(ParamIdx);
			CStrID ParamID = ShaderParam.GetName();

			IPTR MaterialParamIdx = INVALID_INDEX;
			for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
			{
				CTechInfo& TechInfo = UsedTechs[TechIdx];

				IPTR Idx = TechInfo.Params.FindIndex(ParamID);
				if (Idx == INVALID_INDEX) continue;

				CEffectParam& TechParam = TechInfo.Params.ValueAt(Idx);

				if (MaterialParamIdx == INVALID_INDEX)
				{
					MaterialParamIdx = MaterialParams.IndexOf(MaterialParams.Add(TechParam));
					n_msg(VL_DEBUG, "Material param '%s' added\n", ParamID.CStr());

					if (TechParam.Type == EPT_SM40Const)
					{
						U32 BufferRegister = TechParam.pSM40Buffer->Register;
						if (GlobalRegisters.Contains(BufferRegister))
						{
							n_msg(VL_ERROR, "Material param '%s' is placed in a buffer with global params\n", ParamID.CStr());
							return ERR_INVALID_DATA;
						}
						if (!MaterialRegisters.Contains(BufferRegister)) MaterialRegisters.Add(BufferRegister);
					}
					else if (TechParam.Type == EPT_SM30Const)
					{
						CArray<UPTR>& UsedGlobalRegs = (TechParam.pSM30Const->RegisterSet == RS_Float4) ? GlobalFloat4 : ((TechParam.pSM30Const->RegisterSet == RS_Int4) ? GlobalInt4 : GlobalBool);
						CArray<UPTR>& UsedMaterialRegs = (TechParam.pSM30Const->RegisterSet == RS_Float4) ? MaterialFloat4 : ((TechParam.pSM30Const->RegisterSet == RS_Int4) ? MaterialInt4 : MaterialBool);
						for (UPTR r = TechParam.pSM30Const->RegisterStart; r < TechParam.pSM30Const->RegisterStart + TechParam.pSM30Const->RegisterCount; ++r)
						{
							if (UsedGlobalRegs.Contains(r))
							{
								n_msg(VL_ERROR, "Material param '%s' uses a register used by global params\n", ParamID.CStr());
								return ERR_INVALID_DATA;
							}
							if (!UsedMaterialRegs.Contains(r)) UsedMaterialRegs.Add(r);
						}
					}
				}
				else
				{
					CEffectParam& MaterialParam = MaterialParams[MaterialParamIdx];
					if (TechParam != MaterialParam)
					{
						n_msg(VL_ERROR, "Material param '%s' is defined differently in different techs, which is invalid\n", ParamID.CStr());
						return ERR_INVALID_DATA;
					}
				}
			}

			if (MaterialParamIdx == INVALID_INDEX)
			{
				n_msg(VL_WARNING, "Material param '%s' is not used in any valid tech\n", ParamID.CStr());
			}
		}
	}

	// Filter tech-only params (exclude global and material ones)

	for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
	{
		CTechInfo& TechInfo = UsedTechs[TechIdx];
		for (UPTR ParamIdx = 0; ParamIdx < TechInfo.Params.GetCount();)
		{
			CStrID ParamID = TechInfo.Params.KeyAt(ParamIdx);
			CEffectParam& TechParam = TechInfo.Params.ValueAt(ParamIdx);

			if ((GlobalParamsDesc.IsValidPtr() && GlobalParamsDesc->Has(ParamID)) ||
				(MaterialParamsDesc.IsValidPtr() && MaterialParamsDesc->Has(ParamID)))
			{
				TechInfo.Params.RemoveAt(ParamIdx);
				continue;
			}

			n_msg(VL_DEBUG, "Per-object param '%s' added\n", ParamID.CStr());

			if (TechParam.Type == EPT_SM40Const)
			{
				U32 BufferRegister = TechParam.pSM40Buffer->Register;
				if (GlobalRegisters.Contains(BufferRegister))
				{
					n_msg(VL_ERROR, "Tech param '%s' is placed in a buffer with global params\n", ParamID.CStr());
					return ERR_INVALID_DATA;
				}
				if (MaterialRegisters.Contains(BufferRegister))
				{
					n_msg(VL_ERROR, "Tech param '%s' is placed in a buffer with material params\n", ParamID.CStr());
					return ERR_INVALID_DATA;
				}
			}
			else if (TechParam.Type == EPT_SM30Const)
			{
				CArray<UPTR>& UsedGlobalRegs = (TechParam.pSM30Const->RegisterSet == RS_Float4) ? GlobalFloat4 : ((TechParam.pSM30Const->RegisterSet == RS_Int4) ? GlobalInt4 : GlobalBool);
				CArray<UPTR>& UsedMaterialRegs = (TechParam.pSM30Const->RegisterSet == RS_Float4) ? MaterialFloat4 : ((TechParam.pSM30Const->RegisterSet == RS_Int4) ? MaterialInt4 : MaterialBool);
				CArray<UPTR>& UsedTechRegs = (TechParam.pSM30Const->RegisterSet == RS_Float4) ? TechInfo.UsedFloat4 : ((TechParam.pSM30Const->RegisterSet == RS_Int4) ? TechInfo.UsedInt4 : TechInfo.UsedBool);
				for (UPTR r = TechParam.pSM30Const->RegisterStart; r < TechParam.pSM30Const->RegisterStart + TechParam.pSM30Const->RegisterCount; ++r)
				{
					if (UsedGlobalRegs.Contains(r))
					{
						n_msg(VL_ERROR, "Tech param '%s' uses a register used by global params\n", ParamID.CStr());
						return ERR_INVALID_DATA;
					}
					if (UsedMaterialRegs.Contains(r))
					{
						n_msg(VL_ERROR, "Tech param '%s' uses a register used by material params\n", ParamID.CStr());
						return ERR_INVALID_DATA;
					}

					if (!UsedTechRegs.Contains(r)) UsedTechRegs.Add(r);
				}
			}

			++ParamIdx;
		}
	}

	// Write result to a file

	IOSrv->CreateDirectory(PathUtils::ExtractDirName(pOutFilePath));

	IO::PStream File = IOSrv->CreateStream(pOutFilePath);
	if (!File->Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return ERR_IO_WRITE;
	IO::CBinaryWriter W(*File);

	// Save header

	if (!W.Write('SHFX')) return ERR_IO_WRITE;
	if (!W.Write<U32>(0x0100)) return ERR_IO_WRITE;

	// Save render states, each with all light count variations

	if (!W.Write<U32>(UsedRenderStates.GetCount())) return ERR_IO_WRITE;

	for (UPTR i = 0; i < UsedRenderStates.GetCount(); ++i)
	{
		CRenderStateRef& RSRef = UsedRenderStates.ValueAt(i);
		Render::CToolRenderStateDesc& Desc = RSRef.Desc;
		UPTR LightVariationCount = RSRef.MaxLights + 1;

		// Not necessary, passes are referenced by index
		//CStrID ID = UsedRenderStates.KeyAt(i);
		//if (!W.Write(ID)) return ERR_IO_WRITE;

		if (!W.Write<U32>(RSRef.MaxLights)) return ERR_IO_WRITE;
		if (!W.Write<U32>(Desc.Flags.GetMask())) return ERR_IO_WRITE;
		
		if (!W.Write(Desc.DepthBias)) return ERR_IO_WRITE;
		if (!W.Write(Desc.DepthBiasClamp)) return ERR_IO_WRITE;
		if (!W.Write(Desc.SlopeScaledDepthBias)) return ERR_IO_WRITE;
		
		if (Desc.Flags.Is(Render::CToolRenderStateDesc::DS_DepthEnable))
		{
			if (!W.Write<U8>(Desc.DepthFunc)) return ERR_IO_WRITE;
		}
	
		if (Desc.Flags.Is(Render::CToolRenderStateDesc::DS_StencilEnable))
		{
			if (!W.Write(Desc.StencilReadMask)) return ERR_IO_WRITE;
			if (!W.Write(Desc.StencilWriteMask)) return ERR_IO_WRITE;
			if (!W.Write<U32>(Desc.StencilRef)) return ERR_IO_WRITE;

			if (!W.Write<U8>(Desc.StencilFrontFace.StencilFailOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(Desc.StencilFrontFace.StencilDepthFailOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(Desc.StencilFrontFace.StencilPassOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(Desc.StencilFrontFace.StencilFunc)) return ERR_IO_WRITE;

			if (!W.Write<U8>(Desc.StencilBackFace.StencilFailOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(Desc.StencilBackFace.StencilDepthFailOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(Desc.StencilBackFace.StencilPassOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(Desc.StencilBackFace.StencilFunc)) return ERR_IO_WRITE;
		}

		for (UPTR BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
		{
			if (BlendIdx > 0 && Desc.Flags.IsNot(Render::CToolRenderStateDesc::Blend_Independent)) break;
			if (Desc.Flags.IsNot(Render::CToolRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

			const Render::CToolRenderStateDesc::CRTBlend& RTBlend = Desc.RTBlend[BlendIdx];
			if (!W.Write<U8>(RTBlend.SrcBlendArg)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.DestBlendArg)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.BlendOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.SrcBlendArgAlpha)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.DestBlendArgAlpha)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.BlendOpAlpha)) return ERR_IO_WRITE;
			if (!W.Write(RTBlend.WriteMask)) return ERR_IO_WRITE;
		}

		if (!W.Write(Desc.BlendFactorRGBA[0])) return ERR_IO_WRITE;
		if (!W.Write(Desc.BlendFactorRGBA[1])) return ERR_IO_WRITE;
		if (!W.Write(Desc.BlendFactorRGBA[2])) return ERR_IO_WRITE;
		if (!W.Write(Desc.BlendFactorRGBA[3])) return ERR_IO_WRITE;
		if (!W.Write<U32>(Desc.SampleMask)) return ERR_IO_WRITE;

		if (!W.Write(Desc.AlphaTestRef)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Desc.AlphaTestFunc)) return ERR_IO_WRITE;

		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
			for (UPTR ShaderType = Render::ShaderType_Vertex; ShaderType < Render::ShaderType_COUNT; ++ShaderType)
				if (!W.Write<U32>(RSRef.ShaderIDs[ShaderType * LightVariationCount + LightCount])) return ERR_IO_WRITE;
	};

	// Save techniques

	if (!W.Write<U32>(UsedTechs.GetCount())) return ERR_IO_WRITE;

	for (UPTR TechIdx = 0; TechIdx < UsedTechs.GetCount(); ++TechIdx)
	{
		CTechInfo& TechInfo = UsedTechs[TechIdx];

		if (!W.Write(TechInfo.ID)) return ERR_IO_WRITE;
		if (!W.Write(TechInfo.InputSet)) return ERR_IO_WRITE;
		if (!W.Write(TechInfo.Target)) return ERR_IO_WRITE;
		if (!W.Write(TechInfo.MinFeatureLevel)) return ERR_IO_WRITE;
		if (!W.Write(TechInfo.RequiresFlags)) return ERR_IO_WRITE;

		if (!W.Write<U32>(TechInfo.PassIndices.GetCount())) return ERR_IO_WRITE;
		for (UPTR PassIdx = 0; PassIdx < TechInfo.PassIndices.GetCount(); ++PassIdx)
			if (!W.Write<U32>(TechInfo.PassIndices[PassIdx])) return ERR_IO_WRITE;

		UPTR LightVariationCount = TechInfo.MaxLights + 1;

		if (!W.Write<U32>(TechInfo.MaxLights)) return ERR_IO_WRITE;
		for (UPTR LightCount = 0; LightCount < LightVariationCount; ++LightCount)
			if (!W.Write<U8>(TechInfo.VariationValid[LightCount] ? 1 : 0)) return ERR_IO_WRITE;

		// Params are saved already sorted by ID due to CDictionary nature
		if (!W.Write<U32>(TechInfo.Params.GetCount())) return ERR_IO_WRITE;
		for (UPTR ParamIdx = 0; ParamIdx < TechInfo.Params.GetCount(); ++ParamIdx)
		{
			CStrID ParamID = TechInfo.Params.KeyAt(ParamIdx);
			CEffectParam& TechParam = TechInfo.Params.ValueAt(ParamIdx);

			EEffectParamTypeForSaving Type = GetParamTypeForSaving(TechParam.Type);
			if (Type == EPT_Invalid) return ERR_INVALID_DATA;

			if (!W.Write(ParamID)) return ERR_IO_WRITE;
			if (!W.Write<U8>(Type)) return ERR_IO_WRITE;
			if (!W.Write<U8>(TechParam.ShaderType)) return ERR_IO_WRITE;
			if (!W.Write<U32>(TechParam.SourceShaderID)) return ERR_IO_WRITE;
		}

		if (TechInfo.Target < 0x0400)
		{
			WriteRegisterRanges(TechInfo.UsedFloat4, W, "float4");
			WriteRegisterRanges(TechInfo.UsedInt4, W, "int4");
			WriteRegisterRanges(TechInfo.UsedBool, W, "bool");
		}
	}

	// Save global and material params tables

	if (!W.Write<U32>(GlobalParams.GetCount())) return ERR_IO_WRITE;
	for (UPTR ParamIdx = 0; ParamIdx < GlobalParams.GetCount(); ++ParamIdx)
	{
		CEffectParam& Param = GlobalParams[ParamIdx];

		EEffectParamTypeForSaving Type = GetParamTypeForSaving(Param.Type);
		if (Type == EPT_Invalid) return ERR_INVALID_DATA;

		if (!W.Write(Param.ID)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Type)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Param.ShaderType)) return ERR_IO_WRITE;
		if (!W.Write<U32>(Param.SourceShaderID)) return ERR_IO_WRITE;
	}

	//???save SM3.0 global CB info? or only some minimal signature here and all actual data in reference global shader in RP?
	//WriteRegisterRanges(GlobalFloat4, W, "float4");
	//WriteRegisterRanges(GlobalInt4, W, "int4");
	//WriteRegisterRanges(GlobalBool, W, "bool");

	//!!!each material const buffer must be backed in a raw RAM block to set in a GPU material array (part of a big buffer, not a dedicated one)!
	//if dedicated CB is needed, it can be created without RAM back-storage and can be directly transmitted to the GPU, immutable!
	if (!W.Write<U32>(MaterialParams.GetCount())) return ERR_IO_WRITE;
	for (UPTR ParamIdx = 0; ParamIdx < MaterialParams.GetCount(); ++ParamIdx)
	{
		CEffectParam& Param = MaterialParams[ParamIdx];

		EEffectParamTypeForSaving Type = GetParamTypeForSaving(Param.Type);
		if (Type == EPT_Invalid) return ERR_INVALID_DATA;

		if (!W.Write(Param.ID)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Type)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Param.ShaderType)) return ERR_IO_WRITE;
		if (!W.Write<U32>(Param.SourceShaderID)) return ERR_IO_WRITE;

		const Data::CData& DefaultValue = MaterialParamsDesc->Get(Param.ID).GetRawValue();
		if (Type == EPT_Const)
		{
			if (DefaultValue.IsNull()) W.Write(false);
			else
			{
				if (Param.Type == EPT_SM30Const)
				{
					CSM30ShaderConstMeta& Meta = *Param.pSM30Const;
					switch (Meta.RegisterSet)
					{
						case RS_Float4:
						{
							if (DefaultValue.IsA<float>())
							{
								W.Write(true);
								W.Write(1);
								W.Write(DefaultValue.GetValue<float>());
							}
							else if (DefaultValue.IsA<int>())
							{
								W.Write(true);
								W.Write(1);
								W.Write((float)DefaultValue.GetValue<int>());
							}
							else if (DefaultValue.IsA<vector3>())
							{
								const vector3& Vec = DefaultValue.GetValue<vector3>();
								W.Write(true);
								W.Write(3);
								W.Write(Vec);
							}
							else if (DefaultValue.IsA<vector4>())
							{
								const vector4& Vec = DefaultValue.GetValue<vector4>();
								W.Write(true);
								W.Write(4);
								W.Write(Vec);
							}
							else if (DefaultValue.IsA<matrix44>())
							{
								const matrix44& Mtx = DefaultValue.GetValue<matrix44>();
								W.Write(true);
								W.Write(16);
								W.Write(Mtx);
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is a bool, default value must be null, float, int, vector or matrix\n", Param.ID.CStr());
								W.Write(false);
								break;
							}

							n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
							break;
						}
						case RS_Int4:
						{
							if (DefaultValue.IsA<int>())
							{
								W.Write(true);
								W.Write(DefaultValue.GetValue<int>());
								n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is an int, default value must be null or int\n", Param.ID.CStr());
								W.Write(false);
							}
							break;
						}
						case RS_Bool:
						{
							if (DefaultValue.IsA<bool>())
							{
								W.Write(true);
								W.Write(DefaultValue.GetValue<bool>());
								n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is a bool, default value must be null or bool\n", Param.ID.CStr());
								W.Write(false);
							}
							break;
						}
						default:
						{
							n_msg(VL_WARNING, "Material param '%s' has unsupported register set, default value is skipped\n", Param.ID.CStr());
							W.Write(false);
						}
					}
				}
				else if (Param.Type == EPT_SM40Const)
				{
					CD3D11ShaderConstMeta& Meta = *Param.pSM40Const;
					switch (Meta.Type)
					{
						case D3D11Const_Float:
						{
							if (DefaultValue.IsA<float>())
							{
								W.Write(true);
								W.Write(1);
								W.Write(DefaultValue.GetValue<float>());
							}
							else if (DefaultValue.IsA<int>())
							{
								W.Write(true);
								W.Write(1);
								W.Write((float)DefaultValue.GetValue<int>());
							}
							else if (DefaultValue.IsA<vector3>())
							{
								const vector3& Vec = DefaultValue.GetValue<vector3>();
								W.Write(true);
								W.Write(3);
								W.Write(Vec);
							}
							else if (DefaultValue.IsA<vector4>())
							{
								const vector4& Vec = DefaultValue.GetValue<vector4>();
								W.Write(true);
								W.Write(4);
								W.Write(Vec);
							}
							else if (DefaultValue.IsA<matrix44>())
							{
								const matrix44& Mtx = DefaultValue.GetValue<matrix44>();
								W.Write(true);
								W.Write(16);
								W.Write(Mtx);
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is a bool, default value must be null, float, int, vector or matrix\n", Param.ID.CStr());
								W.Write(false);
								break;
							}

							n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
							break;
						}
						case D3D11Const_Int:
						{
							if (DefaultValue.IsA<int>())
							{
								W.Write(true);
								W.Write(DefaultValue.GetValue<int>());
								n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is an int, default value must be null or int\n", Param.ID.CStr());
								W.Write(false);
							}
							break;
						}
						case D3D11Const_Bool:
						{
							if (DefaultValue.IsA<bool>())
							{
								W.Write(true);
								W.Write(DefaultValue.GetValue<bool>());
								n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is a bool, default value must be null or bool\n", Param.ID.CStr());
								W.Write(false);
							}
							break;
						}
						default:
						{
							n_msg(VL_WARNING, "Material param '%s' has unsupported register set, default value is skipped\n", Param.ID.CStr());
							W.Write(false);
						}
					}
				}
				else
				{
					n_msg(VL_WARNING, "Material param '%s' is a constant of unsupported type, default value is skipped\n", Param.ID.CStr());
					W.Write(false);
				}
			}
		}
		else if (Type == EPT_Resource)
		{
			if (DefaultValue.IsA<CStrID>())
			{
				CStrID ResourceID = DefaultValue.GetValue<CStrID>();
				if (ResourceID.IsValid())
				{
					W.Write(true);
					W.Write(ResourceID);
					n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
				}
				else W.Write(false);
			}
			else
			{
				if (!DefaultValue.IsNull())
					n_msg(VL_WARNING, "Material param '%s' is a resource, default value must be null or resource ID of type CStrID\n", Param.ID.CStr());
				W.Write(false);
			}
		}
		else if (Type == EPT_Sampler)
		{
			if (DefaultValue.IsA<Data::PParams>())
			{
				Data::PParams SamplerSection = DefaultValue.GetValue<Data::PParams>();
				if (SamplerSection->GetCount())
				{
					Render::CSamplerDesc SamplerDesc;
					SamplerDesc.SetDefaults();
					if (ProcessSamplerSection(SamplerSection, SamplerDesc))
					{
						W.Write(true);
						
						W.Write<U8>(SamplerDesc.AddressU);
						W.Write<U8>(SamplerDesc.AddressV);
						W.Write<U8>(SamplerDesc.AddressW);
						W.Write<U8>(SamplerDesc.Filter);
						W.Write(SamplerDesc.BorderColorRGBA[0]);
						W.Write(SamplerDesc.BorderColorRGBA[1]);
						W.Write(SamplerDesc.BorderColorRGBA[2]);
						W.Write(SamplerDesc.BorderColorRGBA[3]);
						W.Write(SamplerDesc.MipMapLODBias);
						W.Write(SamplerDesc.FinestMipMapLOD);
						W.Write(SamplerDesc.CoarsestMipMapLOD);
						W.Write<U32>(SamplerDesc.MaxAnisotropy);
						W.Write<U8>(SamplerDesc.CmpFunc);

						n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
					}
					else
					{
						W.Write(false);
						n_msg(VL_WARNING, "Material param '%s' sampler description is invalid, default value is skipped\n", Param.ID.CStr());
					}
				}
				else W.Write(false);
			}
			else
			{
				if (!DefaultValue.IsNull())
					n_msg(VL_WARNING, "Material param '%s' is a sampler, default value must be null or params section\n", Param.ID.CStr());
				W.Write(false);
			}
		}
		else W.Write(false);
	}

	WriteRegisterRanges(MaterialFloat4, W, "float4");
	WriteRegisterRanges(MaterialInt4, W, "int4");
	WriteRegisterRanges(MaterialBool, W, "bool");

	File->Close();

	return SUCCESS;
}
//---------------------------------------------------------------------
