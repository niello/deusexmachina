#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/Streams/MemStream.h>
#include <IO/PathUtils.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <Data/Buffer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>
#include <Data/StringTokenizer.h>
#include <Data/StringUtils.h>
#include <Util/UtilFwd.h>			// CRC
#include <ToolRenderStateDesc.h>	// As a CRenderStateDesc, but shader references are replaced with IDs
#include <ShaderMetadataCache.h>
#include <DEMShaderCompiler/DEMShaderCompilerDLL.h>
#include <Render/SamplerDesc.h>
#include <ConsoleApp.h>

#undef CreateDirectory
#undef DeleteFile

extern CString RootPath;
extern CHashTable<CString, Data::CFourCC> ClassToFOURCC;

//???need?
/*
enum EEffectParamClass
{
	EPC_SM30Const = 0,
	EPC_SM30Resource,
	EPC_SM30Sampler,
	EPC_USMConst,
	EPC_USMResource,
	EPC_USMSampler
};
*/

class CAutoDLLFree
{
private:

	CShaderMetadata* pData;

public:

	CAutoDLLFree(CShaderMetadata* pNewData): pData(pNewData) {}
	~CAutoDLLFree() { if (pData) DLLFreeShaderMetadata(pData); }
};

struct CEffectParam
{
	CStrID				ID;
	Render::EShaderType	ShaderType;
	U32					SourceShaderID;

	// Indirect metadata reference, it remains stable even if arrays inside pMeta are moved in memory
	CShaderMetadata*	pMeta;
	EShaderParamClass	Class;
	UPTR				Index;

	// For default value processing
	EShaderConstType	ConstType;
	U32					SizeInBytes;	// Cached, for consts

	CMetadataObject*		GetMetadataObject() { return pMeta ? pMeta->GetParamObject(Class, Index) : NULL; }
	const CMetadataObject*	GetMetadataObject() const { return pMeta ? pMeta->GetParamObject(Class, Index) : NULL; }
	const CMetadataObject*	GetContainingBuffer() const { return pMeta ? pMeta->GetContainingConstantBuffer(pMeta->GetParamObject(Class, Index)) : NULL; }

	bool IsEqual(const CEffectParam& Other) const
	{
		if (ShaderType != Other.ShaderType) FAIL;

		const CMetadataObject* pMetaObject = GetMetadataObject();
		const CMetadataObject* pOtherMetaObject = Other.GetMetadataObject();
		if (!pMetaObject && !pOtherMetaObject) OK;

		if (!(pMetaObject && pOtherMetaObject && pMetaObject->IsEqual(*pOtherMetaObject))) FAIL;

		if (pMetaObject->GetClass() == ShaderParam_Const && pMetaObject->GetShaderModel() == ShaderModel_USM)
		{
			const CMetadataObject* pBuffer = (const CUSMBufferMeta*)pMeta->GetContainingConstantBuffer(pMetaObject);
			const CMetadataObject* pOtherBuffer = (const CUSMBufferMeta*)Other.pMeta->GetContainingConstantBuffer(pOtherMetaObject);
			return pBuffer->IsEqual(*pOtherBuffer);
			/*
			const CUSMConstMeta* pUSMConst = (const CUSMConstMeta*)pMetaObject;
			U32 MinBufferSize = pUSMConst->Offset + pUSMConst->ElementSize * pUSMConst->ElementCount;

			const CUSMBufferMeta* pUSMBuffer = (const CUSMBufferMeta*)pMeta->GetContainingConstantBuffer(pMetaObject);
			const CUSMBufferMeta* pOtherUSMBuffer = (const CUSMBufferMeta*)Other.pMeta->GetContainingConstantBuffer(pOtherMetaObject);

			return pUSMBuffer->Register == pOtherUSMBuffer->Register &&
				pUSMBuffer->Size >= MinBufferSize &&
				pOtherUSMBuffer->Size >= MinBufferSize;
			*/
		}

		OK;
	}

	bool operator ==(const CEffectParam& Other) const { return IsEqual(Other); }
	bool operator !=(const CEffectParam& Other) const { return !IsEqual(Other); }
};

struct CTechInfo
{
	CStrID					ID;
	CStrID					InputSet;
	UPTR					MaxLights;
	U32						Target;
	U32						MinFeatureLevel;
	U64						RequiresFlags;
	CArray<CStrID>			Passes;
	CFixedArray<UPTR>		PassIndices;
	CFixedArray<bool>		VariationValid;
	CArray<CEffectParam>	Params;

	// For SM3.0 register usage check
	CArray<UPTR>			UsedFloat4;
	CArray<UPTR>			UsedInt4;
	CArray<UPTR>			UsedBool;
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

	CRenderStateRef&	operator =(const CRenderStateRef& Other)
	{
		ID = Other.ID;
		MaxLights = Other.MaxLights;
		Desc = Other.Desc;
		Target = Other.Target;
		MinFeatureLevel = Other.MinFeatureLevel;
		RequiresFlags = Other.RequiresFlags;
		memcpy_s(UsesShader, sizeof(UsesShader), Other.UsesShader, sizeof(Other.UsesShader));
		UPTR DataSize = Render::ShaderType_COUNT * (MaxLights + 1) * sizeof(U32);
		ShaderIDs = (U32*)n_malloc(DataSize);
		memcpy_s(ShaderIDs, DataSize, Other.ShaderIDs, DataSize);
		return *this;
	}

	bool				operator ==(const CRenderStateRef& Other) { return ID == Other.ID; }
};

struct CSortParamsByID
{
	bool operator()(const CEffectParam& a, const CEffectParam& b)
	{
		return strcmp(a.ID.CStr(), b.ID.CStr()) < 0;
	}
};

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

void PrintShaderCompilerMessages(int Result, const char* pPath)
{
	if (Result == DEM_SHADER_COMPILER_SUCCESS)
	{
		const char* pMessages = DLLGetLastOperationMessages();
		if (pMessages) n_msg(VL_WARNING, pMessages);
	}
	else if (Result == DEM_SHADER_COMPILER_COMPILE_ERROR)
	{
		n_msg(VL_ERROR, "Failed to compile '%s' with:\n\n%s\n", pPath, DLLGetLastOperationMessages());
	}
	else if (Result == DEM_SHADER_COMPILER_REFLECTION_ERROR)
	{
		n_msg(VL_ERROR, "	Shader '%s' reflection error\n", pPath);
	}
	else if (Result == DEM_SHADER_COMPILER_IO_WRITE_ERROR)
	{
		n_msg(VL_ERROR, "	Shader '%s' result writing to disk failed\n", pPath);
	}
	else if (Result == DEM_SHADER_COMPILER_DB_ERROR)
	{
		n_msg(VL_ERROR, "	Shader '%s' database operation failed\n", pPath);
	}
	else if (Result >= DEM_SHADER_COMPILER_ERROR)
	{
		n_msg(VL_ERROR, "	Shader '%s' compilation failed because of internal error in a DEMShaderCompiler DLL\n", pPath);
			
		const char* pMessages = DLLGetLastOperationMessages();
		if (pMessages) n_msg(VL_ERROR, pMessages);
	}
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
		int Result = DLLCompileShader(FullSrcPath.CStr(), (EShaderType)ShaderType, (U32)Target, EntryPoint.CStr(), VariationDefines.CStr(), Debug, false, ObjID, SigID);

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
		}

		PrintShaderCompilerMessages(Result, ShortSrcPath.CStr());

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

		// USM

		const char* pBufferTypeStr = "CBuffer";
		const char* pSlotTypeStr = "c";
		UPTR Register = Obj.Register & USMBuffer_RegisterMask;
		if (Obj.Register & USMBuffer_Texture)
		{
			pBufferTypeStr = "TBuffer";
			pSlotTypeStr = "t";
		}
		else if (Obj.Register & USMBuffer_Structured)
		{
			pBufferTypeStr = "SBuffer";
			pSlotTypeStr = "t";
		}
		n_msg(VL_DETAILS, "    %s: %s, %d slot(s) from %s%d\n", pBufferTypeStr, Obj.Name.CStr(), 1, pSlotTypeStr, Register);

		const char* pTypeString = "<unsupported-type>";
		switch (Obj.Type)
		{
			case USMConst_Bool:	pTypeString = "bool"; break;
			case USMConst_Int:	pTypeString = "int"; break;
			case USMConst_Float:	pTypeString = "float"; break;
			case USMConst_Struct:	pTypeString = "struct"; break;
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

	Data::CData RawValue;
	if (BlendSection->Get(RawValue, CStrID("WriteMask")))
	{
		unsigned char Mask = 0;
		if (RawValue.IsA<CString>())
		{
			StrValue = RawValue.GetValue<CString>();
			StrValue.ToLower();
			if (StrValue.FindIndex('r') != INVALID_INDEX) Mask |= Render::ColorMask_Red;
			if (StrValue.FindIndex('g') != INVALID_INDEX) Mask |= Render::ColorMask_Green;
			if (StrValue.FindIndex('b') != INVALID_INDEX) Mask |= Render::ColorMask_Blue;
			if (StrValue.FindIndex('a') != INVALID_INDEX) Mask |= Render::ColorMask_Alpha;
			Blend.WriteMask = Mask;
		}
		else if (RawValue.IsNull() || (RawValue.IsA<int>() && RawValue == 0))
		{
			Blend.WriteMask = 0;
		}
		else
		{
			n_msg(VL_ERROR, "WriteMask is defined improperly\n");
			FAIL;
		}
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

int SaveEffectParams(IO::CBinaryWriter& W, const CArray<CEffectParam>& Params)
{
	UPTR ConstCount = 0;
	UPTR ResourceCount = 0;
	UPTR SamplerCount = 0;
	for (UPTR ParamIdx = 0; ParamIdx < Params.GetCount(); ++ParamIdx)
	{
		CEffectParam& Param = Params[ParamIdx];
		switch (Param.Class)
		{
			case ShaderParam_Const:		++ConstCount; break;
			case ShaderParam_Resource:	++ResourceCount; break;
			case ShaderParam_Sampler:	++SamplerCount; break;
		}
	}

	if (!W.Write<U32>(ConstCount)) return ERR_IO_WRITE;
	for (UPTR ParamIdx = 0; ParamIdx < Params.GetCount(); ++ParamIdx)
	{
		CEffectParam& Param = Params[ParamIdx];
		if (Param.Class != ShaderParam_Const) continue;

		if (!W.Write(Param.ID)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Param.ShaderType)) return ERR_IO_WRITE;
		if (!W.Write<U32>(Param.SourceShaderID)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Param.ConstType)) return ERR_IO_WRITE;
		if (!W.Write<U32>(Param.SizeInBytes)) return ERR_IO_WRITE;
	}

	if (!W.Write<U32>(ResourceCount)) return ERR_IO_WRITE;
	for (UPTR ParamIdx = 0; ParamIdx < Params.GetCount(); ++ParamIdx)
	{
		CEffectParam& Param = Params[ParamIdx];
		if (Param.Class != ShaderParam_Resource) continue;

		if (!W.Write(Param.ID)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Param.ShaderType)) return ERR_IO_WRITE;
		if (!W.Write<U32>(Param.SourceShaderID)) return ERR_IO_WRITE;
	}

	if (!W.Write<U32>(SamplerCount)) return ERR_IO_WRITE;
	for (UPTR ParamIdx = 0; ParamIdx < Params.GetCount(); ++ParamIdx)
	{
		CEffectParam& Param = Params[ParamIdx];
		if (Param.Class != ShaderParam_Sampler) continue;

		if (!W.Write(Param.ID)) return ERR_IO_WRITE;
		if (!W.Write<U8>(Param.ShaderType)) return ERR_IO_WRITE;
		if (!W.Write<U32>(Param.SourceShaderID)) return ERR_IO_WRITE;
	}

	return SUCCESS;
}
//---------------------------------------------------------------------

bool SkipEffectParams(IO::CBinaryReader& Reader)
{
	// Constants
	U32 Count;
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(10, IO::Seek_Current)) FAIL;
	}

	// Resources
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(5, IO::Seek_Current)) FAIL;
	}

	// Samplers
	if (!Reader.Read(Count)) FAIL;
	for (U32 Idx = 0; Idx < Count; ++Idx)
	{
		CString StrValue;
		if (!Reader.Read(StrValue)) FAIL;
		if (!Reader.GetStream().Seek(5, IO::Seek_Current)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

// Header must be already read or skipped
bool EFFSeekToGlobalParams(IO::CStream& Stream)
{
	if (!Stream.IsOpen()) FAIL;

	IO::CBinaryReader R(Stream);

	U32 Count;
	if (!R.Read(Count)) FAIL;
	for (U32 i = 0; i < Count; ++i)
	{
		// Excerpt from a Render::CRenderStateDesc
		enum
		{
			DS_DepthEnable				= 0x00000100,
			DS_StencilEnable			= 0x00000400,
			Blend_Independent			= 0x00010000,	// If not, only RTBlend[0] is used
			Blend_RTBlendEnable			= 0x00020000	// Use (Blend_RTBlendEnable << Index), Index = [0 .. 7]
			// flags from				  0x00020000
			//       to					  0x01000000
			// inclusive are reserved for Blend_RTBlendEnable, 8 bits total
		};

		U32 MaxLights;
		if (!R.Read(MaxLights)) FAIL;
		U32 Flags;
		if (!R.Read(Flags)) FAIL;

		U32 SizeToSkip = 34 + 20 * (MaxLights + 1);
		if (Flags & DS_DepthEnable) SizeToSkip += 1;
		if (Flags & DS_StencilEnable) SizeToSkip += 14;

		for (UPTR BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
		{
			if (BlendIdx > 0 && !(Flags & Blend_Independent)) break;
			SizeToSkip += 1; // WriteMask
			if (!(Flags & (Blend_RTBlendEnable << BlendIdx))) continue;
			SizeToSkip += 6; // Blend fields
		}
		
		if (!Stream.Seek(SizeToSkip, IO::Seek_Current)) FAIL;
	}
	
	if (!R.Read(Count)) FAIL;
	for (U32 i = 0; i < Count; ++i)
	{
		CString StrValue;
		if (!R.Read(StrValue)) FAIL;
		if (!R.Read(StrValue)) FAIL;
		if (!Stream.Seek(16, IO::Seek_Current)) FAIL;
		
		U32 PassCount;
		if (!R.Read(PassCount)) FAIL;
		if (!Stream.Seek(4 * PassCount, IO::Seek_Current)) FAIL;
		
		U32 MaxLights;
		if (!R.Read(MaxLights)) FAIL;
		if (!Stream.Seek(MaxLights + 1, IO::Seek_Current)) FAIL;

		if (!SkipEffectParams(R)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

int CompileEffect(const char* pInFilePath, const char* pOutFilePath, bool Debug, EShaderModel ShaderModel)
{
	// Read effect source file

	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(pInFilePath, Buffer)) return ERR_IO_READ;

	Data::PParams Params;
	{
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), Params)) return ERR_IO_READ;
	}

	CString TypeStr = Params->Get(CStrID("Type"), CString::Empty);
	TypeStr.Trim();
	TypeStr.ToLower();

	U32 EffectType;
	if (TypeStr == "opaque") EffectType = 0;
	else if (TypeStr == "atest" || TypeStr == "alphatest") EffectType = 1;
	else if (TypeStr == "skybox") EffectType = 2;
	else if (TypeStr == "alpha" || TypeStr == "alphablend" || TypeStr == "transparent") EffectType = 3;
	else
	{
		n_msg(VL_WARNING, "Effect type is not specified through an 'EffectType' parameter, use default value 'Other'");
		EffectType = 999;
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
		if (!InputSet.IsValid())
		{
			n_msg(VL_WARNING, "InputSet is not defined for tech '%s', tech is skipped\n", Tech.GetName().CStr());
			continue;
		}
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

	// Metadata objects are allocated in DLL and must be freed through DLL before it is closed
	CShaderMetadataCache MetaCache;

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

		// Process only requested shader model
		const bool IsRequestedShaderModel =
			(ShaderModel == ShaderModel_30 && RSRef.Target < 0x0400) ||
			(ShaderModel == ShaderModel_USM && RSRef.Target >= 0x0400);
		if (!IsRequestedShaderModel)
		{
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
					if (ShaderID == 0) continue;

					CShaderMetadata* pMeta = MetaCache.GetMetadata(ShaderID);
					if (!pMeta)
					{
						n_msg(VL_ERROR, "Failed to load shader metadata for ID %d, binary may be missing or corrupted\n", ShaderID);
						continue;
					}

					// Valid shader found, get its requirements and apply to render state requirements
					AllInvalid = false;

					RSRef.MinFeatureLevel = n_max(RSRef.MinFeatureLevel, pMeta->GetMinFeatureLevel());
					RSRef.RequiresFlags |= pMeta->GetRequiresFlags();
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
	// Each shader stage that uses a param may define it differently

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
					CShaderMetadata* pMeta = MetaCache.GetMetadata(ShaderID);
					if (!pMeta)
					{
						n_msg(VL_ERROR, "Failed to load shader metadata for ID %d, binary may be missing or corrupted\n", ShaderID);
						return ERR_INVALID_DATA;
					}

					//!!!add per-stage support, to map one param to different shader stages simultaneously!
					// If tech param found:
					// Warn if there are more than one stage that parameter uses (warn once, when we add second stage to a param desc)
					// If not registered for this stage, add per-stage metadata
					// If only the identical metadata is allowed in different stages, compare with any already processed stage used as a reference
					// In this case code remains almost unchanged, but param instead of single shader stage stores stage mask.

					// Loop through all constants, resources and samplers in a shader
					// If there is no effect param with the same name, add new param
					// Else check compatibility of the effect param and fail if incompatible

					for (UPTR ShaderParamClassIdx = ShaderParam_Const; ShaderParamClassIdx < ShaderParam_COUNT; ++ShaderParamClassIdx)
					{
						EShaderParamClass ShaderParamClass = (EShaderParamClass)ShaderParamClassIdx;
						UPTR ParamCount = pMeta->GetParamCount(ShaderParamClass);
						for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
						{
							CMetadataObject* pMetaObject = pMeta->GetParamObject(ShaderParamClass, ParamIdx);
							CStrID MetaObjectID = CStrID(pMetaObject->GetName());

							// Try to find existing effect param by name ID
							UPTR Idx = 0;
							for (; Idx < TechInfo.Params.GetCount(); ++ Idx)
								if (TechInfo.Params[Idx].ID == MetaObjectID) break;

							if (Idx == TechInfo.Params.GetCount())
							{
								// Not found, create new param

								CEffectParam& Param = *TechInfo.Params.Reserve(1);
								Param.ID = MetaObjectID;
								Param.ShaderType = (Render::EShaderType)ShaderType;
								Param.SourceShaderID = ShaderID;

								Param.pMeta = pMeta;
								Param.Class = ShaderParamClass;
								Param.Index = ParamIdx;

								CString BufferString;

								if (ShaderParamClass == ShaderParam_Const)
								{
									CMetadataObject* pMetaBuffer = pMeta->GetContainingConstantBuffer(pMetaObject);
									BufferString.Format(" (buffer '%s')", pMetaBuffer->GetName());
									
									EShaderModel MetaObjectModel = pMetaObject->GetShaderModel();
									switch (MetaObjectModel)
									{
										case ShaderModel_30:
										{
											const CSM30ConstMeta& MetaObj = (const CSM30ConstMeta&)pMetaObject;
											switch (MetaObj.RegisterSet)
											{
												case RS_Float4:
												{
													Param.ConstType = ShaderConst_Float;
													Param.SizeInBytes = 4 * sizeof(float) * MetaObj.ElementRegisterCount * MetaObj.ElementCount;
													break;
												}
												case RS_Int4:
												{
													Param.ConstType = ShaderConst_Int;
													Param.SizeInBytes = sizeof(I32) * MetaObj.ElementRegisterCount * MetaObj.ElementCount; // * 4 - if int4!
													break;
												}
												case RS_Bool:
												{
													Param.ConstType = ShaderConst_Bool;
													Param.SizeInBytes = sizeof(bool) * MetaObj.ElementRegisterCount * MetaObj.ElementCount;
													break;
												}
											}
											break;
										}
										case ShaderModel_USM:
										{
											const CUSMConstMeta& MetaObj = (const CUSMConstMeta&)pMetaObject;
											switch (MetaObj.Type)
											{
												case USMConst_Bool:		Param.ConstType = ShaderConst_Bool; break;
												case USMConst_Int:		Param.ConstType = ShaderConst_Int; break;
												case USMConst_Float:	Param.ConstType = ShaderConst_Float; break;
												case USMConst_Struct:	Param.ConstType = ShaderConst_Struct; break;
												case USMConst_Invalid:	Param.ConstType = ShaderConst_Invalid; break;
											}
											Param.SizeInBytes = MetaObj.ElementSize * MetaObj.ElementCount;
											break;
										}
									}
								}

								// Debug output
								const char* pClassName = "";
								switch (ShaderParamClass)
								{
									case ShaderParam_Const:
										pClassName = "const";
										break;
									case ShaderParam_Resource:
										pClassName = "resource";
										break;
									case ShaderParam_Sampler:
										pClassName = "sampler";
										break;
								}
								n_msg(VL_DEBUG, "Tech '%s': %s '%s' added%s\n", TechInfo.ID.CStr(), pClassName, MetaObjectID.CStr(), BufferString.IsValid() ? BufferString.CStr() : "");
							}
							else
							{
								// Found, check compatibility

								const CEffectParam& Param = TechInfo.Params[Idx];
								const CMetadataObject* pExistingMetaObject = Param.GetMetadataObject();
								
								n_assert(pExistingMetaObject);
								n_assert(pExistingMetaObject->GetShaderModel() == pMetaObject->GetShaderModel());

								if (!pMetaObject->IsEqual(*pExistingMetaObject))
								{
									n_msg(VL_ERROR, "Tech '%s': param '%s' has different description in different shaders\n", TechInfo.ID.CStr(), MetaObjectID.CStr());
									return ERR_INVALID_DATA;
								}

								if (ShaderParamClass == ShaderParam_Const)
								{
									const CMetadataObject* pMetaBuffer = pMeta->GetContainingConstantBuffer(pMetaObject);
									const CMetadataObject* pExistingBuffer = Param.GetContainingBuffer();
									n_assert(pMetaBuffer && pExistingBuffer);
									if (!pMetaBuffer->IsEqual(*pExistingBuffer)) //???must be equal or compatible?
									{
										n_msg(VL_ERROR, "Tech '%s': param '%s' containing buffers have different description in different shaders\n", TechInfo.ID.CStr(), MetaObjectID.CStr());
										return ERR_INVALID_DATA;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// Build global and material param tables

	CArray<CEffectParam> GlobalParams;
	CArray<CEffectParam> MaterialParams;

	// USM
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

				UPTR Idx = 0;
				for (; Idx < TechInfo.Params.GetCount(); ++ Idx)
					if (TechInfo.Params[Idx].ID == ParamID) break;
				if (Idx == TechInfo.Params.GetCount()) continue;

				CEffectParam& TechParam = TechInfo.Params[Idx];

				if (GlobalParamIdx == INVALID_INDEX)
				{
					GlobalParamIdx = GlobalParams.IndexOf(GlobalParams.Add(TechParam));
					n_msg(VL_DEBUG, "Global param '%s' added\n", ParamID.CStr());

					if (TechParam.Class == ShaderParam_Const)
					{
						CMetadataObject* pMetaObject = TechParam.GetMetadataObject();
						EShaderModel ParamModel = pMetaObject->GetShaderModel();
						if (ParamModel == ShaderModel_USM)
						{
							U32 BufferRegister = ((CUSMBufferMeta*)TechParam.GetContainingBuffer())->Register;
							if (MaterialRegisters.Contains(BufferRegister))
							{
								n_msg(VL_ERROR, "Global param '%s' is placed in a buffer with material params\n", ParamID.CStr());
								return ERR_INVALID_DATA;
							}
							if (!GlobalRegisters.Contains(BufferRegister)) GlobalRegisters.Add(BufferRegister);
						}
						else if (ParamModel == ShaderModel_30)
						{
							CSM30ConstMeta* pSM30Const = (CSM30ConstMeta*)pMetaObject;
							CArray<UPTR>& UsedGlobalRegs = (pSM30Const->RegisterSet == RS_Float4) ? GlobalFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? GlobalInt4 : GlobalBool);
							CArray<UPTR>& UsedMaterialRegs = (pSM30Const->RegisterSet == RS_Float4) ? MaterialFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? MaterialInt4 : MaterialBool);
							for (UPTR r = pSM30Const->RegisterStart; r < pSM30Const->RegisterStart + pSM30Const->RegisterCount; ++r)
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
				}
				else
				{
					CEffectParam& GlobalParam = GlobalParams[GlobalParamIdx];
					if (TechParam != GlobalParam)
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

				UPTR Idx = 0;
				for (; Idx < TechInfo.Params.GetCount(); ++ Idx)
					if (TechInfo.Params[Idx].ID == ParamID) break;
				if (Idx == TechInfo.Params.GetCount()) continue;

				CEffectParam& TechParam = TechInfo.Params[Idx];

				if (MaterialParamIdx == INVALID_INDEX)
				{
					MaterialParamIdx = MaterialParams.IndexOf(MaterialParams.Add(TechParam));
					n_msg(VL_DEBUG, "Material param '%s' added\n", ParamID.CStr());

					if (TechParam.Class == ShaderParam_Const)
					{
						CMetadataObject* pMetaObject = TechParam.GetMetadataObject();
						EShaderModel ParamModel = pMetaObject->GetShaderModel();
						if (ParamModel == ShaderModel_USM)
						{
							U32 BufferRegister = ((CUSMBufferMeta*)TechParam.GetContainingBuffer())->Register;
							if (GlobalRegisters.Contains(BufferRegister))
							{
								n_msg(VL_ERROR, "Material param '%s' is placed in a buffer with global params\n", ParamID.CStr());
								return ERR_INVALID_DATA;
							}
							if (!MaterialRegisters.Contains(BufferRegister)) MaterialRegisters.Add(BufferRegister);
						}
						else if (ParamModel == ShaderModel_30)
						{
							CSM30ConstMeta* pSM30Const = (CSM30ConstMeta*)pMetaObject;
							CArray<UPTR>& UsedGlobalRegs = (pSM30Const->RegisterSet == RS_Float4) ? GlobalFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? GlobalInt4 : GlobalBool);
							CArray<UPTR>& UsedMaterialRegs = (pSM30Const->RegisterSet == RS_Float4) ? MaterialFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? MaterialInt4 : MaterialBool);
							for (UPTR r = pSM30Const->RegisterStart; r < pSM30Const->RegisterStart + pSM30Const->RegisterCount; ++r)
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
		for (UPTR ParamIdx = 0; ParamIdx < TechInfo.Params.GetCount(); /*incremented inside*/)
		{
			CEffectParam& TechParam = TechInfo.Params[ParamIdx];
			CStrID ParamID = TechParam.ID;

			if ((GlobalParamsDesc.IsValidPtr() && GlobalParamsDesc->Has(ParamID)) ||
				(MaterialParamsDesc.IsValidPtr() && MaterialParamsDesc->Has(ParamID)))
			{
				TechInfo.Params.RemoveAt(ParamIdx);
				continue;
			}

			n_msg(VL_DEBUG, "Per-object param '%s' added\n", ParamID.CStr());

			if (TechParam.Class == ShaderParam_Const)
			{
				CMetadataObject* pMetaObject = TechParam.GetMetadataObject();
				EShaderModel ParamModel = pMetaObject->GetShaderModel();
				if (ParamModel == ShaderModel_USM)
				{
					U32 BufferRegister = ((CUSMBufferMeta*)TechParam.GetContainingBuffer())->Register;
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
				else if (ParamModel == ShaderModel_30)
				{
					CSM30ConstMeta* pSM30Const = (CSM30ConstMeta*)pMetaObject;
					CArray<UPTR>& UsedGlobalRegs = (pSM30Const->RegisterSet == RS_Float4) ? GlobalFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? GlobalInt4 : GlobalBool);
					CArray<UPTR>& UsedMaterialRegs = (pSM30Const->RegisterSet == RS_Float4) ? MaterialFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? MaterialInt4 : MaterialBool);
					CArray<UPTR>& UsedTechRegs = (pSM30Const->RegisterSet == RS_Float4) ? TechInfo.UsedFloat4 : ((pSM30Const->RegisterSet == RS_Int4) ? TechInfo.UsedInt4 : TechInfo.UsedBool);
					for (UPTR r = pSM30Const->RegisterStart; r < pSM30Const->RegisterStart + pSM30Const->RegisterCount; ++r)
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
	if (!W.Write<U32>(ShaderModel)) return ERR_IO_WRITE;
	if (!W.Write<U32>(EffectType)) return ERR_IO_WRITE;

	// Save render states, each with all light count variations

	if (!W.Write<U32>(UsedRenderStates.GetCount())) return ERR_IO_WRITE;
	for (UPTR i = 0; i < UsedRenderStates.GetCount(); ++i)
	{
		CRenderStateRef& RSRef = UsedRenderStates.ValueAt(i);
		Render::CToolRenderStateDesc& Desc = RSRef.Desc;
		UPTR LightVariationCount = RSRef.MaxLights + 1;

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
			
			const Render::CToolRenderStateDesc::CRTBlend& RTBlend = Desc.RTBlend[BlendIdx];
			if (!W.Write<U8>(RTBlend.WriteMask)) return ERR_IO_WRITE;
			
			if (Desc.Flags.IsNot(Render::CToolRenderStateDesc::Blend_RTBlendEnable << BlendIdx)) continue;

			if (!W.Write<U8>(RTBlend.SrcBlendArg)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.DestBlendArg)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.BlendOp)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.SrcBlendArgAlpha)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.DestBlendArgAlpha)) return ERR_IO_WRITE;
			if (!W.Write<U8>(RTBlend.BlendOpAlpha)) return ERR_IO_WRITE;
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

	// Sort techniques

	// In each InputSet sort from the richest to the lightest tech. When loading an effect,
	// application will choose the first suitable tech and then can skip remaining ones.
	struct CSortTechsByInputSetAndFeatures
	{
		bool operator()(const CTechInfo& a, const CTechInfo& b)
		{
			if (a.InputSet != b.InputSet) return a.InputSet < b.InputSet;
			if (a.Target != b.Target) return a.Target > b.Target;
			if (a.MinFeatureLevel != b.MinFeatureLevel) return a.MinFeatureLevel > b.MinFeatureLevel;
			UPTR AFlagCount = 0;
			for (UPTR i = 0; i < sizeof(a.RequiresFlags) * 8; ++i)
				if (a.RequiresFlags & (1i64 << i)) ++AFlagCount;
			UPTR BFlagCount = 0;
			for (UPTR i = 0; i < sizeof(b.RequiresFlags) * 8; ++i)
				if (b.RequiresFlags & (1i64 << i)) ++BFlagCount;
			if (AFlagCount != BFlagCount) return AFlagCount > BFlagCount;
			return a.MaxLights > b.MaxLights;
		}
	};
	UsedTechs.Sort<CSortTechsByInputSetAndFeatures>();

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

		TechInfo.Params.Sort<CSortParamsByID>();
		int SaveResult = SaveEffectParams(W, TechInfo.Params);
		if (SaveResult != SUCCESS) return SaveResult;
	}

	// Save global params table

	GlobalParams.Sort<CSortParamsByID>();
	int SaveResult = SaveEffectParams(W, GlobalParams);
	if (SaveResult != SUCCESS) return SaveResult;

	// Save material params table

	MaterialParams.Sort<CSortParamsByID>();
	SaveResult = SaveEffectParams(W, MaterialParams);
	if (SaveResult != SUCCESS) return SaveResult;

	// Save material param default values: param ID, class, value or offset in a value buffer

	IO::PMemStream DefaultConstValues = n_new(IO::CMemStream);
	DefaultConstValues->Open(IO::SAM_READWRITE, IO::SAP_SEQUENTIAL);

	U64 DefValCountPos = W.GetStream().GetPosition();
	U32 DefValCount = 0;
	if (!W.Write<U32>(0)) return ERR_IO_WRITE;
	for (UPTR ParamIdx = 0; ParamIdx < MaterialParams.GetCount(); ++ParamIdx)
	{
		CEffectParam& Param = MaterialParams[ParamIdx];
		
		const Data::CData& DefaultValue = MaterialParamsDesc->Get(Param.ID).GetRawValue();
		if (DefaultValue.IsNull()) continue;

		const CMetadataObject* pMetaObject = Param.GetMetadataObject();
		EShaderModel ParamModel = pMetaObject->GetShaderModel();
		if (Param.Class == ShaderParam_Const)
		{
			EShaderConstType ConstType = ShaderConst_Invalid;
			U32 ConstSizeInBytes = Param.SizeInBytes;
			U32 ValueSizeInBytes = 0;
			U32 CurrDefValOffset = (U32)DefaultConstValues->GetPosition();
			
			if (ParamModel == ShaderModel_30)
			{
				const CSM30ConstMeta& MetaObj = *(const CSM30ConstMeta*)pMetaObject;
				switch (MetaObj.RegisterSet)
				{
					case RS_Float4:	ConstType = ShaderConst_Float; break;
					case RS_Int4:	ConstType = ShaderConst_Int; break;
					case RS_Bool:	ConstType = ShaderConst_Bool; break;
				}
			}
			else if (ParamModel == ShaderModel_USM)
			{
				const CUSMConstMeta& MetaObj = *(const CUSMConstMeta*)pMetaObject;
				switch (MetaObj.Type)
				{
					case USMConst_Bool:		ConstType = ShaderConst_Bool; break;
					case USMConst_Int:		ConstType = ShaderConst_Int; break;
					case USMConst_Float:	ConstType = ShaderConst_Float; break;
					case USMConst_Struct:	ConstType = ShaderConst_Struct; break;
					case USMConst_Invalid:	ConstType = ShaderConst_Invalid; break;
				}
			}

			switch (ConstType)
			{
				case ShaderConst_Float:
				{
					if (DefaultValue.IsA<float>())
					{
						float DefValue = DefaultValue.GetValue<float>();
						ValueSizeInBytes = n_min(sizeof(float), ConstSizeInBytes);
						DefaultConstValues->Write(&DefValue, ValueSizeInBytes);
					}
					else if (DefaultValue.IsA<int>())
					{
						float DefValue = (float)DefaultValue.GetValue<int>();
						ValueSizeInBytes = n_min(sizeof(float), ConstSizeInBytes);
						DefaultConstValues->Write(&DefValue, ValueSizeInBytes);
					}
					else if (DefaultValue.IsA<vector3>())
					{
						const vector3& DefValue = DefaultValue.GetValue<vector3>();
						ValueSizeInBytes = n_min(sizeof(vector3), ConstSizeInBytes);
						DefaultConstValues->Write(DefValue.v, ValueSizeInBytes);
					}
					else if (DefaultValue.IsA<vector4>())
					{
						const vector4& DefValue = DefaultValue.GetValue<vector4>();
						ValueSizeInBytes = n_min(sizeof(vector4), ConstSizeInBytes);
						DefaultConstValues->Write(DefValue.v, ValueSizeInBytes);
					}
					else if (DefaultValue.IsA<matrix44>())
					{
						const matrix44& DefValue = DefaultValue.GetValue<matrix44>();
						ValueSizeInBytes = n_min(sizeof(matrix44), ConstSizeInBytes);
						DefaultConstValues->Write(DefValue.m, ValueSizeInBytes);
					}
					else
					{
						n_msg(VL_WARNING, "Material param '%s' is a float, default value must be null, float, int, vector or matrix\n", Param.ID.CStr());
						continue;
					}

					break;
				}
				case ShaderConst_Int:
				{
					if (DefaultValue.IsA<int>())
					{
						I32 DefValue = (I32)DefaultValue.GetValue<int>();
						ValueSizeInBytes = n_min(sizeof(I32), ConstSizeInBytes);
						DefaultConstValues->Write(&DefValue, ValueSizeInBytes);
					}
					else
					{
						n_msg(VL_WARNING, "Material param '%s' is an int, default value must be null or int\n", Param.ID.CStr());
						continue;
					}
					break;
				}
				case ShaderConst_Bool:
				{
					if (DefaultValue.IsA<bool>())
					{
						bool DefValue = DefaultValue.GetValue<bool>();
						ValueSizeInBytes = n_min(sizeof(bool), ConstSizeInBytes);
						DefaultConstValues->Write(&DefValue, ValueSizeInBytes);
					}
					else
					{
						n_msg(VL_WARNING, "Material param '%s' is a bool, default value must be null or bool\n", Param.ID.CStr());
						continue;
					}
					break;
				}
				default:
				{
					n_msg(VL_WARNING, "Material param '%s' is a constant of unsupported type or register set, default value is skipped\n", Param.ID.CStr());
					continue;
				}
			}
			
			if (ConstSizeInBytes > ValueSizeInBytes) DefaultConstValues->Fill(0, ConstSizeInBytes - ValueSizeInBytes);
			
			if (!W.Write(Param.ID)) return ERR_IO_WRITE;
			if (!W.Write<U8>(ShaderParam_Const)) return ERR_IO_WRITE;
			if (!W.Write<U32>(CurrDefValOffset)) return ERR_IO_WRITE;
			++DefValCount;
			n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
		}
		else if (Param.Class == ShaderParam_Resource)
		{
			CString ResourceID;
			if (DefaultValue.IsA<CStrID>()) ResourceID = CString(DefaultValue.GetValue<CStrID>().CStr());
			else if (DefaultValue.IsA<CString>()) ResourceID = DefaultValue.GetValue<CString>();
			else
			{
				n_msg(VL_WARNING, "Material param '%s' is a resource, default value must be null or resource ID of type CString or CStrID\n", Param.ID.CStr());
				continue;
			}

			if (ResourceID.IsValid())
			{
				if (!W.Write(Param.ID)) return ERR_IO_WRITE;
				if (!W.Write<U8>(ShaderParam_Resource)) return ERR_IO_WRITE;
				if (!W.Write(ResourceID)) return ERR_IO_WRITE;
				++DefValCount;
				n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
			}
		}
		else if (Param.Class == ShaderParam_Sampler)
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
						if (!W.Write(Param.ID)) return ERR_IO_WRITE;
						if (!W.Write<U8>(ShaderParam_Sampler)) return ERR_IO_WRITE;
						
						if (!W.Write<U8>(SamplerDesc.AddressU)) return ERR_IO_WRITE;
						if (!W.Write<U8>(SamplerDesc.AddressV)) return ERR_IO_WRITE;
						if (!W.Write<U8>(SamplerDesc.AddressW)) return ERR_IO_WRITE;
						if (!W.Write<U8>(SamplerDesc.Filter)) return ERR_IO_WRITE;
						if (!W.Write(SamplerDesc.BorderColorRGBA[0])) return ERR_IO_WRITE;
						if (!W.Write(SamplerDesc.BorderColorRGBA[1])) return ERR_IO_WRITE;
						if (!W.Write(SamplerDesc.BorderColorRGBA[2])) return ERR_IO_WRITE;
						if (!W.Write(SamplerDesc.BorderColorRGBA[3])) return ERR_IO_WRITE;
						if (!W.Write(SamplerDesc.MipMapLODBias)) return ERR_IO_WRITE;
						if (!W.Write(SamplerDesc.FinestMipMapLOD)) return ERR_IO_WRITE;
						if (!W.Write(SamplerDesc.CoarsestMipMapLOD)) return ERR_IO_WRITE;
						if (!W.Write<U32>(SamplerDesc.MaxAnisotropy)) return ERR_IO_WRITE;
						if (!W.Write<U8>(SamplerDesc.CmpFunc)) return ERR_IO_WRITE;

						++DefValCount;
						
						n_msg(VL_DEBUG, "Material param '%s' default value processed\n", Param.ID.CStr());
					}
					else n_msg(VL_WARNING, "Material param '%s' sampler description is invalid, default value is skipped\n", Param.ID.CStr());
				}
			}
			else n_msg(VL_WARNING, "Material param '%s' is a sampler, default value must be null or params section\n", Param.ID.CStr());
		}
	}

	// Write an actual default value count
	U64 CurrPos = W.GetStream().GetPosition();
	W.GetStream().Seek(DefValCountPos, IO::Seek_Begin);
	if (!W.Write(DefValCount)) return ERR_IO_WRITE;
	W.GetStream().Seek(CurrPos, IO::Seek_Begin);

	U32 DefaultConstValuesSize = (U32)DefaultConstValues->GetSize();
	if (!W.Write(DefaultConstValuesSize)) return ERR_IO_WRITE;
	if (DefaultConstValuesSize)
	{
		if (W.GetStream().Write(DefaultConstValues->Map(), DefaultConstValuesSize) != DefaultConstValuesSize) return ERR_IO_WRITE;
		DefaultConstValues->Unmap();
	}
	DefaultConstValues = NULL;

	File->Close();

	return SUCCESS;
}
//---------------------------------------------------------------------

int ProcessGlobalEffectParam(IO::CBinaryReader& R,
							 EShaderParamClass Class,
							 CArray<CEffectParam>& GlobalParams,
							 CShaderMetadataCache& MetaCache,
							 CDict<U64, U32>& StructIndexMapping,
							 CShaderMetadata* pGlobalMeta)
{
	// Load API-independent effect parameter description

	CEffectParam Param;
	if (!R.Read(Param.ID)) return ERR_IO_READ;

	U8 ShaderType;
	if (!R.Read(ShaderType)) return ERR_IO_READ;
	Param.ShaderType = (Render::EShaderType)ShaderType;

	if (!R.Read(Param.SourceShaderID)) return ERR_IO_READ;	

	const U32 ShaderID = Param.SourceShaderID;

	if (Class == ShaderParam_Const)
	{
		U8 ConstType;
		if (!R.Read<U8>(ConstType)) return ERR_IO_READ;
		Param.ConstType = (EShaderConstType)ConstType;
		if (!R.Read<U32>(Param.SizeInBytes)) return ERR_IO_READ;
	}

	// Load API-specific parameter metadata

	CShaderMetadata* pMeta = MetaCache.GetMetadata(ShaderID);
	if (!pMeta)
	{
		n_msg(VL_ERROR, "Failed to load shader metadata for ID %d, binary may be missing or corrupted\n", ShaderID);
		return ERR_INVALID_DATA;
	}

	if (pMeta->GetShaderModel() != pGlobalMeta->GetShaderModel())
	{
		n_msg(VL_ERROR, "Render path mixes effects with different shader models in globals section\n");
		return ERR_INVALID_DATA;
	}

	// Extend requirements of global metadata with requirements of the current shader
	pGlobalMeta->SetMinFeatureLevel(n_max(pGlobalMeta->GetMinFeatureLevel(), pMeta->GetMinFeatureLevel()));
	pGlobalMeta->SetRequiresFlags(pGlobalMeta->GetRequiresFlags() | pMeta->GetRequiresFlags());

	// Fill shader parameter metadata of an effect parameter
	UPTR Idx;
	if (!pMeta->FindParamObjectByName(Class, Param.ID.CStr(), Idx))
	{
		n_msg(VL_ERROR, "Parameter '%s' not found in shader %d\n", Param.ID.CStr(), ShaderID);
		return ERR_INVALID_DATA;
	}
	Param.pMeta = pMeta;
	Param.Class = Class;
	Param.Index = Idx;

	// Add a parameter to the list, verify its compatibility across all referenced effects

	CEffectParam* pAddedParam = NULL;

	Idx = 0;
	for (; Idx < GlobalParams.GetCount(); ++ Idx)
		if (GlobalParams[Idx].ID == Param.ID) break;
	if (Idx == GlobalParams.GetCount())
	{
		// Not found in the list, add new global param
		pAddedParam = GlobalParams.Add(Param);

		// No source shader is used, all metadata will be included into an RP file
		pAddedParam->SourceShaderID = 0;

		// Add parameter metadata from source shader to global metadata collection
		pAddedParam->pMeta = pGlobalMeta;
		pAddedParam->Index = pGlobalMeta->AddParamObject(Class, Param.GetMetadataObject());
		if (pAddedParam->Index == (UPTR)(INVALID_INDEX))
		{
			n_msg(VL_ERROR, "Failed to add param '%s' to global metadata\n", Param.ID.CStr());
			return ERR_INVALID_DATA;
		}
	}
	else
	{
		// Found in the list, compare with existing to verify compatibility
		pAddedParam = &GlobalParams[Idx];

		const CMetadataObject* pExistingMeta = Param.GetMetadataObject();
		const CMetadataObject* pAddedMeta = pAddedParam->GetMetadataObject();

		EShaderModel CurrModel = pExistingMeta->GetShaderModel();
		n_assert(CurrModel == pAddedMeta->GetShaderModel());

		if (!pExistingMeta->IsEqual(*pAddedMeta))
		{
			n_msg(VL_ERROR, "Global param '%s' has different description in different effects\n", Param.ID.CStr());
			return ERR_INVALID_DATA;
		}

		// Check buffer compatibility for constants, given that we can extend global buffers if required,
		// so only binding address (register / slot) matters

		if (Param.Class == ShaderParam_Const)
		{
			const CMetadataObject* pExistingBuffer = Param.GetContainingBuffer();
			const CMetadataObject* pAddedBuffer = pAddedParam->GetContainingBuffer();
			if (CurrModel == ShaderModel_USM)
			{
				const CUSMBufferMeta* pExistingUSMBuffer = (const CUSMBufferMeta*)pExistingBuffer;
				const CUSMBufferMeta* pAddedUSMBuffer = (const CUSMBufferMeta*)pAddedBuffer;
				if (pExistingUSMBuffer->Register != pAddedUSMBuffer->Register)
				{
					n_msg(VL_ERROR, "Global param '%s' containing buffer is bound to different registers (0x%x %s:b%d, 0x%x %s:b%d) in different effects\n",
						  Param.ID.CStr(), pExistingUSMBuffer, pExistingUSMBuffer->Name.CStr(), pExistingUSMBuffer->Register,
						  pAddedUSMBuffer, pAddedUSMBuffer->Name.CStr(), pAddedUSMBuffer->Register);
					return ERR_INVALID_DATA;
				}
			}
			else if (CurrModel == ShaderModel_30)
			{
				const CSM30BufferMeta* pExistingSM30Buffer = (const CSM30BufferMeta*)pExistingBuffer;
				const CSM30BufferMeta* pAddedSM30Buffer = (const CSM30BufferMeta*)pAddedBuffer;
				if (pExistingSM30Buffer->SlotIndex != pAddedSM30Buffer->SlotIndex)
				{
					n_msg(VL_ERROR, "Global param '%s' containing buffer is bound to different slot indices in different effects\n", Param.ID.CStr());
					return ERR_INVALID_DATA;
				}
			}
		}
	}

	// For resources and samplers we have done
	if (Param.Class != ShaderParam_Const) return SUCCESS;

	UPTR AddedConstIndex = pAddedParam->Index;

	// Add buffer containing this constant. If buffer already exist at this register, merge them.

	const CMetadataObject* pMetaBuffer = Param.GetContainingBuffer();
	UPTR BufferIdx = pGlobalMeta->AddOrMergeBuffer(pMetaBuffer);
	if (BufferIdx == (UPTR)INVALID_INDEX)
	{
		n_msg(VL_ERROR, "Failed to add or merge constant buffer for const '%s'\n", Param.ID.CStr());
		return ERR_INVALID_DATA;
	}
	pGlobalMeta->SetContainingConstantBuffer(AddedConstIndex, BufferIdx);

	// Add structure layout of this constant if it is a structure, process nested structures

	U32 StructIndex = pMeta->GetStructureIndex(Param.Index);
	if (StructIndex != (U32)(-1))
	{
		const U64 StructKey = (((U64)ShaderID) << 32) | ((U64)StructIndex);
		U32 GlobalStructIndex = pGlobalMeta->AddStructure(*pMeta, StructKey, StructIndexMapping);
		pGlobalMeta->SetStructureIndex(AddedConstIndex, GlobalStructIndex);
	}

	return SUCCESS;
}
//---------------------------------------------------------------------

int CompileRenderPath(const char* pInFilePath, const char* pOutFilePath, EShaderModel ShaderModel)
{
	// Read render path source file

	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(pInFilePath, Buffer)) return ERR_IO_READ;

	Data::PParams Params;
	{
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((const char*)Buffer.GetPtr(), Buffer.GetSize(), Params)) return ERR_IO_READ;
	}

	// Process phases

	Data::PParams Phases;
	if (!Params->Get(Phases, CStrID("Phases"))) return ERR_INVALID_DATA;
	for (UPTR i = 0; i < Phases->GetCount(); ++i)
	{
		Data::PParams PhaseDesc = Phases->Get<Data::PParams>(i);
		Data::PDataArray Renderers;
		if (!PhaseDesc->Get(Renderers, CStrID("Renderers"))) continue;

		// Replace class names with FourCC codes where possible
		for (UPTR j = 0; j < Renderers->GetCount(); ++j)
		{
			Data::PParams Elm = Renderers->Get<Data::PParams>(j);
			Data::CFourCC Value;

			const CString& ObjectClassName = Elm->Get<CString>(CStrID("Object"));
			if (ClassToFOURCC.Get(ObjectClassName, Value)) Elm->Set<int>(CStrID("Object"), (int)Value.Code);

			const CString& RendererClassName = Elm->Get<CString>(CStrID("Renderer"));
			if (ClassToFOURCC.Get(RendererClassName, Value)) Elm->Set<int>(CStrID("Renderer"), (int)Value.Code);
		}
	}

	// Build globals table and accompanying metadata

	CArray<CEffectParam> GlobalParams;

	CShaderMetadata* pGlobalMeta;
	DLLCreateShaderMetadata(ShaderModel, pGlobalMeta);
	if (!pGlobalMeta) return ERR_INVALID_DATA;
	CAutoDLLFree GlobalMetaScopedKiller(pGlobalMeta);

	Data::PDataArray EffectsWithGlobals;
	if (Params->Get(EffectsWithGlobals, CStrID("EffectsWithGlobals")))
	{
		CShaderMetadataCache MetaCache;
		CDict<U64, U32> StructIndexMapping;			// Key is (ShaderID << 32) | StructIndex

		// All referenced effects must be already exported
		//???or compile them here?
		for (UPTR i = 0; i < EffectsWithGlobals->GetCount(); ++i)
		{
			const Data::CData& Value = EffectsWithGlobals->Get(i);
			const char* pEffectID;
			if (Value.IsA<CString>()) pEffectID = Value.GetValue<CString>().CStr();
			else if (Value.IsA<CStrID>()) pEffectID = Value.GetValue<CStrID>().CStr();
			else continue;

			CString EffectPath("Effects:");
			EffectPath += pEffectID;
			EffectPath += ".eff";
			
			IO::PStream EFF = IOSrv->CreateStream(EffectPath);
			if (!EFF->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL))
			{
				n_msg(VL_INFO, "Effect '%s' skipped because its file '%s' is not found\n", pEffectID, EffectPath.CStr());
				continue;
			}

			IO::CBinaryReader R(*EFF.GetUnsafe());

			U32 U32Value;
			if (!R.Read(U32Value)) return ERR_IO_READ;
			if (U32Value != 'SHFX') return ERR_INVALID_DATA;	// Magic
			if (!R.Read(U32Value)) return ERR_IO_READ;
			if (U32Value != 0x0100) return ERR_INVALID_DATA;	// Version, fail if unsupported
			U32 EffectShaderModel;
			if (!R.Read(EffectShaderModel)) return ERR_IO_READ;	// Shader model
			if (!R.Read(U32Value)) return ERR_IO_READ;			// Effect type

			if (EffectShaderModel != ShaderModel)
			{
				EFF->Close();
				n_msg(VL_INFO, "Effect '%s' skipped because its shader model doesn't match a requested one\n", pEffectID);
				continue;
			}
			
			if (!EFFSeekToGlobalParams(*EFF.GetUnsafe())) return ERR_INVALID_DATA;

			U32 GlobalCount;
			if (!R.Read(GlobalCount)) return ERR_IO_READ;
			for (U32 i = 0; i < GlobalCount; ++i)
			{
				int CallResult = ProcessGlobalEffectParam(R, ShaderParam_Const, GlobalParams, MetaCache, StructIndexMapping, pGlobalMeta);
				if (CallResult != SUCCESS) return CallResult;
			}

			if (!R.Read(GlobalCount)) return ERR_IO_READ;
			for (U32 i = 0; i < GlobalCount; ++i)
			{
				int CallResult = ProcessGlobalEffectParam(R, ShaderParam_Resource, GlobalParams, MetaCache, StructIndexMapping, pGlobalMeta);
				if (CallResult != SUCCESS) return CallResult;
			}

			if (!R.Read(GlobalCount)) return ERR_IO_READ;
			for (U32 i = 0; i < GlobalCount; ++i)
			{
				int CallResult = ProcessGlobalEffectParam(R, ShaderParam_Sampler, GlobalParams, MetaCache, StructIndexMapping, pGlobalMeta);
				if (CallResult != SUCCESS) return CallResult;
			}
		}
	}

	// Write result to a file

	if (!IOSrv->CreateDirectory(PathUtils::ExtractDirName(pOutFilePath))) return ERR_IO_WRITE;

	IO::PStream File = IOSrv->CreateStream(pOutFilePath);
	if (!File->Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return ERR_IO_WRITE;
	IO::CBinaryWriter W(*File);

	// Save header

	if (!W.Write('RPTH')) return ERR_IO_WRITE;
	if (!W.Write<U32>(0x0100)) return ERR_IO_WRITE;
	if (!W.Write<U32>(ShaderModel)) return ERR_IO_WRITE;

	// Save pluggable object slots and rendering phases

	Data::PDataArray DataToSave;
	Params->Get(DataToSave, CStrID("RenderTargets"));
	if (!W.Write(DataToSave)) return ERR_IO_WRITE;
	Params->Get(DataToSave, CStrID("DepthStencilBuffers"));
	if (!W.Write(DataToSave)) return ERR_IO_WRITE;

	if (!W.WriteParams(*Phases)) return ERR_IO_WRITE;

	// Save global params

	if (!pGlobalMeta->Save(W)) return ERR_IO_WRITE;

	GlobalParams.Sort<CSortParamsByID>();
	int SaveResult = SaveEffectParams(W, GlobalParams);
	if (SaveResult != SUCCESS) return SaveResult;

	File->Close();

	return SUCCESS;
}
//---------------------------------------------------------------------
