#include "Main.h"

#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
#include <IO/IOServer.h>
#include <IO/FSBrowser.h>
#include <IO/Streams/FileStream.h>
#include <IO/Streams/MemStream.h>
#include <IO/BinaryReader.h>
#include <IO/BinaryWriter.h>
#include <IO/PathUtils.h>
#include <Data/DataServer.h>
#include <Data/DataArray.h>
#include <DEMShaderCompiler/DEMShaderCompilerDLL.h>

void ConvertPropNamesToFourCC(Data::PDataArray Props)
{
	for (UPTR i = 0; i < Props->GetCount(); ++i)
	{
		if (!Props->Get(i).IsA<CString>()) continue;
		const CString& Name = Props->Get<CString>(i);
		Data::CFourCC Value;
		if (ClassToFOURCC.Get(Name, Value)) Props->At(i) = (int)Value.Code;
	}
}
//---------------------------------------------------------------------

bool ProcessDialogue(const CString& SrcContext, const CString& ExportContext, const CString& Name)
{
	CString ExportFilePath = ExportContext + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".hrd", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	bool ScriptExported = false;

	// Script can implements functions like OnStart(), so export anyway
	const CString& ScriptFile = Desc->Get<CString>(CStrID("Script"), CString::Empty);
	if (ScriptFile.IsValid())
	{
		ExportFilePath = CString("Scripts:") + ScriptFile + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), CString("SrcScripts:") + ScriptFile + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessCollisionShape(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	// Add terrain file for heightfield shapes (always exported) //???or allow building from L3DT src?
	CString CDLODFile = Desc->Get(CStrID("CDLODFile"), CString::Empty);
	if (CDLODFile.IsValid())
	{
		CString CDLODFilePath = "Terrain:" + CDLODFile + ".cdlod";
		if (!IsFileAdded(CDLODFilePath))
		{
			if (ExportResources &&
				!ProcessResourceDesc("SrcTerrain:" + CDLODFile + ".cfd", CDLODFilePath) &&
				!IOSrv->FileExists(CDLODFilePath))
			{
				n_msg(VL_ERROR, "Referenced resource '%s' doesn't exist and isn't exportable through CFD\n", PathUtils::GetExtension(CDLODFilePath));
				FAIL;
			}
			FilesToPack.InsertSorted(CDLODFilePath);
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessPhysicsDesc(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	Data::PDataArray Objects;
	if (Desc->Get(Objects, CStrID("Objects")))
	{
		for (UPTR i = 0; i < Objects->GetCount(); ++i)
		{
			Data::PParams ObjDesc = Objects->Get<Data::PParams>(i);
			CStrID PickShape = ObjDesc->Get<CStrID>(CStrID("Shape"), CStrID::Empty);
			if (PickShape.IsValid())
				if (!ProcessCollisionShape(	CString("SrcPhysics:") + PickShape.CStr() + ".hrd",
											CString("Physics:") + PickShape.CStr() + ".prm"))
				{
					n_msg(VL_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
					FAIL;
				}
		}
	}


	OK;
}
//---------------------------------------------------------------------

bool ProcessAnimDesc(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	for (UPTR i = 0; i < Desc->GetCount(); ++i)
	{
		//???!!!allow compile or batch-compile?
		// can add Model resource description, associated with this anim, to CFModel list
		CString FileName = CString("Anims:") + Desc->Get(i).GetValue<CStrID>().CStr();
		if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
	}

	OK;
}
//---------------------------------------------------------------------

//???pre-unwind descs on exports?
bool ProcessDescWithParents(const CString& SrcContext, const CString& ExportContext, const CString& Name)
{
	CString ExportFilePath = ExportContext + Name + ".prm";

	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcContext + Name + ".hrd", false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	CString BaseName = Desc->Get(CStrID("_Base_"), CString::Empty);
	return BaseName.IsEmpty() || (BaseName != Name && ProcessDescWithParents(SrcContext, ExportContext, BaseName));
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

// For materials
bool EFFSeekToMaterialParams(IO::CStream& Stream)
{
	if (!Stream.IsOpen()) FAIL;

	IO::CBinaryReader R(Stream);

	U32 U32Value;
	if (!R.Read(U32Value) || U32Value != 'SHFX') FAIL;	// Magic
	if (!R.Read(U32Value) || U32Value != 0x0100) FAIL;	// Version, fail if unsupported
	if (!R.Read(U32Value)) FAIL;						// Shader model

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
			if (!(Flags & (Blend_RTBlendEnable << BlendIdx))) continue;
			SizeToSkip += 7;
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

	if (!SkipEffectParams(R)) FAIL;

	OK;
}
//---------------------------------------------------------------------

// For materials //!!!DUPLICATE CODE! see CFShader
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

// For materials //!!!DUPLICATE CODE! see CFShader
Render::ETexAddressMode StringToTexAddressMode(const CString& Str)
{
	if (Str == "mirror") return Render::TexAddr_Mirror;
	if (Str == "clamp") return Render::TexAddr_Clamp;
	if (Str == "border") return Render::TexAddr_Border;
	if (Str == "mirroronce") return Render::TexAddr_MirrorOnce;
	return Render::TexAddr_Wrap;
}
//---------------------------------------------------------------------

// For materials //!!!DUPLICATE CODE! see CFShader
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

// For materials //!!!DUPLICATE CODE! see CFShader
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

bool ProcessEffect(const char* pExportFileName)
{
	IO::PStream EFF = IOSrv->CreateStream(pExportFileName);
	if (!EFF->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader R(*EFF.GetUnsafe());

	U32 U32Value;
	if (!R.Read(U32Value) || U32Value != 'SHFX') FAIL;	// Magic
	if (!R.Read(U32Value) || U32Value != 0x0100) FAIL;	// Version, fail if unsupported
	if (!R.Read(U32Value)) FAIL;						// Shader model

	U32 Count;
	if (!R.Read(Count)) FAIL;
	for (U32 i = 0; i < Count; ++i)
	{
		U32 MaxLights;
		if (!R.Read(MaxLights)) FAIL;
		U32 Flags;
		if (!R.Read(Flags)) FAIL;

		U32 SizeToSkip = 34;
		if (Flags & Render::CRenderStateDesc::DS_DepthEnable) SizeToSkip += 1;
		if (Flags & Render::CRenderStateDesc::DS_StencilEnable) SizeToSkip += 14;

		for (UPTR BlendIdx = 0; BlendIdx < 8; ++BlendIdx)
		{
			if (BlendIdx > 0 && !(Flags & Render::CRenderStateDesc::Blend_Independent)) break;
			if (!(Flags & (Render::CRenderStateDesc::Blend_RTBlendEnable << BlendIdx))) continue;
			SizeToSkip += 7;
		}
		
		if (!EFF->Seek(SizeToSkip, IO::Seek_Current)) FAIL;

		// Pack referenced shaders
		UPTR ShaderCount = Render::ShaderType_COUNT * (MaxLights + 1);
		for (UPTR ShaderIdx = 0; ShaderIdx < ShaderCount; ++ShaderIdx)
		{
			U32 ShaderID;
			if (!R.Read(ShaderID)) FAIL;
			if (ShaderID != 0) AddShaderToPack(ShaderID);
		}
	}

	// Skip techs
	if (!R.Read(Count)) FAIL;
	for (U32 i = 0; i < Count; ++i)
	{
		CString StrValue;
		if (!R.Read(StrValue)) FAIL;
		if (!R.Read(StrValue)) FAIL;
		if (!EFF->Seek(16, IO::Seek_Current)) FAIL;
		
		U32 PassCount;
		if (!R.Read(PassCount)) FAIL;
		if (!EFF->Seek(4 * PassCount, IO::Seek_Current)) FAIL;
		
		U32 MaxLights;
		if (!R.Read(MaxLights)) FAIL;
		if (!EFF->Seek(MaxLights + 1, IO::Seek_Current)) FAIL;

		if (!SkipEffectParams(R)) FAIL;
	}

	if (!SkipEffectParams(R)) FAIL;
	if (!SkipEffectParams(R)) FAIL;

	// Gather textures from default values
	U32 DefValCount;
	if (!R.Read(DefValCount)) FAIL;
	for (U32 i = 0; i < DefValCount; ++i)
	{
		CString StrValue;
		if (!R.Read(StrValue)) FAIL;

		U8 Type;
		if (!R.Read(Type)) FAIL;

		if (Type == 0) // Constant
		{
			if (!EFF->Seek(4, IO::Seek_Current)) FAIL;
		}
		else if (Type == 1) // Resource
		{
			if (!R.Read(StrValue)) FAIL;
				
			if (ExportResources)
			{
				//!!!export from src, find resource desc and add source texture to CFTexture list!
			}
				
			CString TexExportFileName = StrValue;
			if (!IsFileAdded(TexExportFileName)) FilesToPack.InsertSorted(TexExportFileName);
		}
		else if (Type == 2) // Sampler
		{
			if (!EFF->Seek(37, IO::Seek_Current)) FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessMaterialDesc(const char* pName)
{
	CString ExportFilePath("Materials:");
	ExportFilePath += pName;
	ExportFilePath += ".mtl";

	if (IsFileAdded(ExportFilePath)) OK;

	// Will be set to true if we detect that an effect was not parsed yet
	bool GatherEffectReferencesUSM = false;
	bool GatherEffectReferencesSM30 = false;

	// Since texture sources are referenced only in desc sources,
	// we must export descs even if only texture export is required.
	if (ExportDescs || ExportResources)
	{
		Data::PParams Desc = DataSrv->LoadHRD(CString("SrcMaterials:") + pName + ".hrd", false);
		if (!Desc.IsValidPtr()) FAIL;

		CStrID EffectID = Desc->Get(CStrID("Effect"), CStrID::Empty);
		if (!EffectID.IsValid())
		{
			n_msg(VL_WARNING, "Material '%s' refereces no effect, skipped\n", pName);
			FAIL;
		}

		CString EffectSrcFileName("SrcShaders:Effects/");
		EffectSrcFileName += EffectID.CStr();
		EffectSrcFileName += ".hrd";

		CString EffectExportFileNameUSM("Shaders:USM/Effects/");
		EffectExportFileNameUSM += EffectID.CStr();
		EffectExportFileNameUSM += ".eff";

		CString EffectExportFileNameSM30("Shaders:SM_3_0/Effects/");
		EffectExportFileNameSM30 += EffectID.CStr();
		EffectExportFileNameSM30 += ".eff";

		//!!!must export textures inside ExportEffect() if requested! now we have no info about texture source in EFF!
		//BBuilder & CFShader conflict at the texture/effect/material exporting task
		//Since effect is a desc, it must be exported from BBuilder for correct reference processing!
		//Or CFShader will be responsible for exporting EFF default textures from source.
		//Also we may parse EFF desc here, export textures and then export effect.

		if (!IsFileAdded(EffectExportFileNameUSM))
		{
			if (!ExportEffect(EffectSrcFileName, EffectExportFileNameUSM, false)) FAIL;
			FilesToPack.InsertSorted(EffectExportFileNameUSM);
			GatherEffectReferencesUSM = true;
		}

		if (IncludeSM30ShadersAndEffects && !IsFileAdded(EffectExportFileNameSM30))
		{
			if (!ExportEffect(EffectSrcFileName, EffectExportFileNameSM30, true)) FAIL;
			FilesToPack.InsertSorted(EffectExportFileNameSM30);
			GatherEffectReferencesSM30 = true;
		}

		// Load EFF material param table

		IO::PStream EFF = IOSrv->CreateStream(EffectExportFileNameUSM.CStr());
		if (!EFF->Open(IO::SAM_READ, IO::SAP_RANDOM)) FAIL;
		if (!EFFSeekToMaterialParams(*EFF.GetUnsafe())) FAIL;

		struct CConstInfo
		{
			U8	Type;
			U32	SizeInBytes;
		};

		CDict<CStrID, CConstInfo> MtlConsts;
		CArray<CStrID> MtlResources;
		CArray<CStrID> MtlSamplers;

		IO::CBinaryReader R(*EFF.GetUnsafe());

		U32 ParamCount;
		if (!R.Read<U32>(ParamCount)) FAIL;
		for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
		{
			CStrID ParamID;
			if (!R.Read(ParamID)) FAIL;
			U8 ShaderType;
			if (!R.Read(ShaderType)) FAIL;
			U32 SourceShaderID;
			if (!R.Read(SourceShaderID)) FAIL;

			U8 Type;
			if (!R.Read(Type)) FAIL;
			U32 SizeInBytes;
			if (!R.Read(SizeInBytes)) FAIL;

			MtlConsts.Add(ParamID, { Type, SizeInBytes });
		}

		if (!R.Read<U32>(ParamCount)) FAIL;
		for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
		{
			CStrID ParamID;
			if (!R.Read(ParamID)) FAIL;
			U8 ShaderType;
			if (!R.Read(ShaderType)) FAIL;
			U32 SourceShaderID;
			if (!R.Read(SourceShaderID)) FAIL;
			
			MtlResources.Add(ParamID);
		}

		if (!R.Read<U32>(ParamCount)) FAIL;
		for (UPTR ParamIdx = 0; ParamIdx < ParamCount; ++ParamIdx)
		{
			CStrID ParamID;
			if (!R.Read(ParamID)) FAIL;
			U8 ShaderType;
			if (!R.Read(ShaderType)) FAIL;
			U32 SourceShaderID;
			if (!R.Read(SourceShaderID)) FAIL;

			MtlSamplers.Add(ParamID);
		}

		EFF->Close();

		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));

		// Save material mtl

		IO::PStream File = IOSrv->CreateStream(ExportFilePath);
		if (!File->Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) FAIL;
		IO::CBinaryWriter W(*File);

		if (!W.Write('MTRL')) FAIL;
		if (!W.Write<U32>(0x0100)) FAIL;

		if (!W.Write(EffectID)) FAIL;

		Data::PParams ParamValuesDesc;
		if (Desc->Get(ParamValuesDesc, CStrID("Params")))
		{
			IO::PMemStream ConstValues = n_new(IO::CMemStream);
			ConstValues->Open(IO::SAM_READWRITE, IO::SAP_SEQUENTIAL);

			U64 ValCountPos = W.GetStream().GetPosition();
			U32 ValCount = 0;
			if (!W.Write<U32>(0)) FAIL; // Value count will be written later, when it is calculated
			for (UPTR ParamIdx = 0; ParamIdx < ParamValuesDesc->GetCount(); ++ParamIdx)
			{
				CStrID ParamID = ParamValuesDesc->Get(ParamIdx).GetName();
				const Data::CData& Value = ParamValuesDesc->Get(ParamIdx).GetRawValue();
				if (Value.IsNull()) continue;

				if (MtlConsts.Contains(ParamID)) // Constant
				{
					const CConstInfo& ConstInfo = MtlConsts[ParamID];
					U8 ConstType = ConstInfo.Type;
					U32 ConstSizeInBytes = ConstInfo.SizeInBytes;
					U32 ValueSizeInBytes = 0;
					U32 CurrDefValOffset = (U32)ConstValues->GetPosition();

					switch (ConstType)
					{
						case USMConst_Float:
						{
							if (Value.IsA<float>())
							{
								float TypedValue = Value.GetValue<float>();
								ValueSizeInBytes = n_min(sizeof(float), ConstSizeInBytes);
								ConstValues->Write(&TypedValue, ValueSizeInBytes);
							}
							else if (Value.IsA<int>())
							{
								float TypedValue = (float)Value.GetValue<int>();
								ValueSizeInBytes = n_min(sizeof(float), ConstSizeInBytes);
								ConstValues->Write(&TypedValue, ValueSizeInBytes);
							}
							else if (Value.IsA<vector3>())
							{
								const vector3& TypedValue = Value.GetValue<vector3>();
								ValueSizeInBytes = n_min(sizeof(vector3), ConstSizeInBytes);
								ConstValues->Write(TypedValue.v, ValueSizeInBytes);
							}
							else if (Value.IsA<vector4>())
							{
								const vector4& TypedValue = Value.GetValue<vector4>();
								ValueSizeInBytes = n_min(sizeof(vector4), ConstSizeInBytes);
								ConstValues->Write(TypedValue.v, ValueSizeInBytes);
							}
							else if (Value.IsA<matrix44>())
							{
								const matrix44& TypedValue = Value.GetValue<matrix44>();
								ValueSizeInBytes = n_min(sizeof(matrix44), ConstSizeInBytes);
								ConstValues->Write(TypedValue.m, ValueSizeInBytes);
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is a bool, value must be null, float, int, vector or matrix\n", ParamID.CStr());
								continue;
							}

							break;
						}
						case USMConst_Int:
						{
							if (Value.IsA<int>())
							{
								I32 TypedValue = (I32)Value.GetValue<int>();
								ValueSizeInBytes = n_min(sizeof(I32), ConstSizeInBytes);
								ConstValues->Write(&TypedValue, ValueSizeInBytes);
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is an int, value must be null or int\n", ParamID.CStr());
								continue;
							}
							break;
						}
						case USMConst_Bool:
						{
							if (Value.IsA<bool>())
							{
								bool TypedValue = Value.GetValue<bool>();
								ValueSizeInBytes = n_min(sizeof(bool), ConstSizeInBytes);
								ConstValues->Write(&TypedValue, ValueSizeInBytes);
							}
							else
							{
								n_msg(VL_WARNING, "Material param '%s' is a bool, value must be null or bool\n", ParamID.CStr());
								continue;
							}
							break;
						}
						default:
						{
							n_msg(VL_WARNING, "Material param '%s' is a constant of unsupported type or register set, value is skipped\n", ParamID.CStr());
							continue;
						}
					}
			
					if (ConstSizeInBytes > ValueSizeInBytes) ConstValues->Fill(0, ConstSizeInBytes - ValueSizeInBytes);
			
					if (!W.Write(ParamID)) FAIL;
					if (!W.Write<U8>(0)) FAIL;
					if (!W.Write<U32>(CurrDefValOffset)) FAIL;
					++ValCount;
					n_msg(VL_DEBUG, "Material param '%s' value processed\n", ParamID.CStr());
				}
				else if (MtlResources.Contains(ParamID)) // Resource
				{
					CString ResourceID;
					if (Value.IsA<CStrID>()) ResourceID = CString(Value.GetValue<CStrID>().CStr());
					else if (Value.IsA<CString>()) ResourceID = Value.GetValue<CString>();
					else
					{
						n_msg(VL_WARNING, "Material param '%s' is a resource, value must be null or resource ID of type CString or CStrID\n", ParamID.CStr());
						continue;
					}

					if (ResourceID.IsValid())
					{
						if (!W.Write(ParamID)) FAIL;
						if (!W.Write<U8>(1)) FAIL;
						if (!W.Write(ResourceID)) FAIL;
						++ValCount;
						n_msg(VL_DEBUG, "Material param '%s' value processed\n", ParamID.CStr());

						if (ExportResources)
						{
							//!!!export from src, find resource desc and add source texture to CFTexture list!
						}
					
						CString TexExportFileName = ResourceID;
						if (!IsFileAdded(TexExportFileName)) FilesToPack.InsertSorted(TexExportFileName);
					}
				}
				else if (MtlSamplers.Contains(ParamID)) // Sampler
				{
					if (Value.IsA<Data::PParams>())
					{
						Data::PParams SamplerSection = Value.GetValue<Data::PParams>();
						if (SamplerSection->GetCount())
						{
							Render::CSamplerDesc SamplerDesc;
							SamplerDesc.SetDefaults();
							if (ProcessSamplerSection(SamplerSection, SamplerDesc))
							{
								if (!W.Write(ParamID)) FAIL;
								if (!W.Write<U8>(2)) FAIL;
						
								if (!W.Write<U8>(SamplerDesc.AddressU)) FAIL;
								if (!W.Write<U8>(SamplerDesc.AddressV)) FAIL;
								if (!W.Write<U8>(SamplerDesc.AddressW)) FAIL;
								if (!W.Write<U8>(SamplerDesc.Filter)) FAIL;
								if (!W.Write(SamplerDesc.BorderColorRGBA[0])) FAIL;
								if (!W.Write(SamplerDesc.BorderColorRGBA[1])) FAIL;
								if (!W.Write(SamplerDesc.BorderColorRGBA[2])) FAIL;
								if (!W.Write(SamplerDesc.BorderColorRGBA[3])) FAIL;
								if (!W.Write(SamplerDesc.MipMapLODBias)) FAIL;
								if (!W.Write(SamplerDesc.FinestMipMapLOD)) FAIL;
								if (!W.Write(SamplerDesc.CoarsestMipMapLOD)) FAIL;
								if (!W.Write<U32>(SamplerDesc.MaxAnisotropy)) FAIL;
								if (!W.Write<U8>(SamplerDesc.CmpFunc)) FAIL;

								++ValCount;
						
								n_msg(VL_DEBUG, "Material param '%s' value processed\n", ParamID.CStr());
							}
							else n_msg(VL_WARNING, "Material param '%s' sampler description is invalid, value is skipped\n", ParamID.CStr());
						}
					}
					else n_msg(VL_WARNING, "Material param '%s' is a sampler, value must be null or params section\n", ParamID.CStr());
				}
			}

			// Write an actual value count
			U64 CurrPos = W.GetStream().GetPosition();
			W.GetStream().Seek(ValCountPos, IO::Seek_Begin);
			if (!W.Write(ValCount)) FAIL;
			W.GetStream().Seek(CurrPos, IO::Seek_Begin);

			U32 ConstValuesSize = (U32)ConstValues->GetSize();
			if (!W.Write(ConstValuesSize)) FAIL;
			if (ConstValuesSize)
			{
				if (W.GetStream().Write(ConstValues->Map(), ConstValuesSize) != ConstValuesSize) FAIL;
				ConstValues->Unmap();
			}
		}

		File->Close();

		FilesToPack.InsertSorted(ExportFilePath);
	}

	// Process .mtl binary

	IO::PStream MTL = IOSrv->CreateStream(ExportFilePath);
	if (!MTL->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) FAIL;
	IO::CBinaryReader RMtl(*MTL.GetUnsafe());

	U32 U32Value;
	if (!RMtl.Read(U32Value) || U32Value != 'MTRL') FAIL;	// Magic
	if (!RMtl.Read(U32Value) || U32Value != 0x0100) FAIL;	// Version, fail if unsupported

	CStrID EffectID;
	if (!RMtl.Read(EffectID)) FAIL;	// Magic

	CString EffectExportFileNameUSM("Shaders:USM/Effects/");
	EffectExportFileNameUSM += EffectID.CStr();
	EffectExportFileNameUSM += ".eff";

	CString EffectExportFileNameSM30("Shaders:SM_3_0/Effects/");
	EffectExportFileNameSM30 += EffectID.CStr();
	EffectExportFileNameSM30 += ".eff";

	// Gather referenced effect and textures from a material, if weren't packed when exporting MTL
	if (!ExportDescs && !ExportResources)
	{
		if (!IsFileAdded(EffectExportFileNameUSM))
		{
			FilesToPack.InsertSorted(EffectExportFileNameUSM);
			GatherEffectReferencesUSM = true;
		}

		if (IncludeSM30ShadersAndEffects && !IsFileAdded(EffectExportFileNameSM30))
		{
			FilesToPack.InsertSorted(EffectExportFileNameSM30);
			GatherEffectReferencesSM30 = true;
		}

		U32 ParamCount;
		if (!RMtl.Read(ParamCount)) FAIL;
		for (U32 i = 0; i < ParamCount; ++i)
		{
			CString StrValue;
			if (!RMtl.Read(StrValue)) FAIL;

			U8 Type;
			if (!RMtl.Read(Type)) FAIL;

			if (Type == 0) // Constant
			{
				if (!MTL->Seek(4, IO::Seek_Current)) FAIL;
			}
			else if (Type == 1) // Resource
			{
				if (!RMtl.Read(StrValue)) FAIL;
				
				if (ExportResources)
				{
					//!!!export from src, find resource desc and add source texture to CFTexture list!
				}
				
				CString TexExportFileName = StrValue;
				if (!IsFileAdded(TexExportFileName)) FilesToPack.InsertSorted(TexExportFileName);
			}
			else if (Type == 2) // Sampler
			{
				if (!MTL->Seek(37, IO::Seek_Current)) FAIL;
			}
		}
	}

	// Gather referenced shaders and textures from effect default values

	if (GatherEffectReferencesUSM)
	{
		if (!ProcessEffect(EffectExportFileNameUSM)) FAIL;
	}

	if (GatherEffectReferencesSM30)
	{
		if (!ProcessEffect(EffectExportFileNameSM30)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessRenderPath(const char* pExportFilePath)
{
	// No external resources referenced here for now
	OK;
}
//---------------------------------------------------------------------

bool ProcessSceneNodeRefs(const Data::CParams& NodeDesc)
{
	Data::PDataArray Attrs;
	if (NodeDesc.Get(Attrs, CStrID("Attrs")))
	{
		for (UPTR i = 0; i < Attrs->GetCount(); ++i)
		{
			Data::PParams AttrDesc = Attrs->Get<Data::PParams>(i);

			Data::PParams Textures;
			if (AttrDesc->Get(Textures, CStrID("Textures")))
			{
				//!!!when export from src, find resource desc and add source texture to CFTexture list!
				for (UPTR i = 0; i < Textures->GetCount(); ++i)
				{
					CString FileName = CString("Textures:") + Textures->Get(i).GetValue<CStrID>().CStr();
					if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
				}
			}

			Data::CData* pValue;

			if (AttrDesc->Get(pValue, CStrID("Material")))
				if (!ProcessMaterialDesc(pValue->GetValue<CStrID>().CStr())) FAIL;

			//!!!when export from src, find resource desc and add source Model to CFModel list!
			if (AttrDesc->Get(pValue, CStrID("Mesh")))
			{
				CString FileName = CString("Meshes:") + pValue->GetValue<CStrID>().CStr() + ".nvx2"; //!!!change extension!
				if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
			}

			if (AttrDesc->Get(pValue, CStrID("SkinInfo")))
			{
				CString FileName = CString("Scene:") + pValue->GetValue<CStrID>().CStr() + ".skn";
				if (!IsFileAdded(FileName)) FilesToPack.InsertSorted(FileName);
			}

			//!!!when export from src, find resource desc and add source BT to CFTerrain list!
			if (AttrDesc->Get(pValue, CStrID("CDLODFile")))
			{
				CString CDLODFilePath = "Terrain:" + pValue->GetValue<CString>() + ".cdlod";
				if (!IsFileAdded(CDLODFilePath))
				{
					if (ExportResources &&
						!ProcessResourceDesc("SrcTerrain:" + pValue->GetValue<CString>() + ".cfd", CDLODFilePath) &&
						!IOSrv->FileExists(CDLODFilePath))
					{
						n_msg(VL_ERROR, "Referenced resource '%s' doesn't exist and isn't exportable through CFD\n", PathUtils::GetExtension(CDLODFilePath));
						FAIL;
					}
					FilesToPack.InsertSorted(CDLODFilePath);
				}
			}
		}
	}

	Data::PParams Children;
	if (NodeDesc.Get(Children, CStrID("Children")))
		for (UPTR i = 0; i < Children->GetCount(); ++i)
			if (!ProcessSceneNodeRefs(*Children->Get(i).GetValue<Data::PParams>())) FAIL;

	OK;
}
//---------------------------------------------------------------------

// Always processed from Src
bool ProcessSceneResource(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc = DataSrv->LoadHRD(SrcFilePath, false);
	if (!Desc.IsValidPtr()) FAIL;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		IO::PStream File = IOSrv->CreateStream(ExportFilePath);
		if (!File->Open(IO::SAM_WRITE)) FAIL;
		IO::CBinaryWriter Writer(*File);
		Writer.WriteParams(*Desc, *DataSrv->GetDataScheme(CStrID("SceneNode")));
		File->Close();
	}

	FilesToPack.InsertSorted(ExportFilePath);

	return ProcessSceneNodeRefs(*Desc);
}
//---------------------------------------------------------------------

bool ProcessUISettingsDesc(const char* pSrcFilePath, const char* pExportFilePath)
{
	//???where to define? in a desc (in a shader section)?
	bool CompileDebugShaders = false;

	// Some descs can be loaded twice or more from different IO path assigns, avoid it
	CString RealExportFilePath = IOSrv->ResolveAssigns(pExportFilePath);

	if (IsFileAdded(RealExportFilePath)) OK;

	Data::PParams UIDesc = DataSrv->LoadHRD(pSrcFilePath, false);
	if (UIDesc.IsNullPtr()) OK; // No UI desc

	Data::PParams ResourceGroupsDesc;
	if (UIDesc->Get<Data::PParams>(ResourceGroupsDesc, CStrID("ResourceGroups")))
	{
		for (UPTR i = 0; i < ResourceGroupsDesc->GetCount(); ++i)
		{
			const Data::CParam& RsrcGroup = ResourceGroupsDesc->Get(i);
			const CString& DestDir = RsrcGroup.GetValue<CString>();

			if (ExportResources)
			{
				CString SrcDir = "Src" + DestDir;

				if (!IOSrv->DirectoryExists(SrcDir))
				{
					n_msg(VL_ERROR, "UI resource directory '%s' referenced in '%s' doesn't exist\n", SrcDir.CStr(), pSrcFilePath);
					FAIL;
				}

				BatchToolInOut(CStrID("CFCopy"), SrcDir, DestDir);
			}

			if (!IsFileAdded(DestDir)) FilesToPack.InsertSorted(DestDir);
		}
	}

	if (ExportDescs)
	{
		Data::PParams ShadersDesc;
		if (UIDesc->Get<Data::PParams>(ShadersDesc, CStrID("Shaders")))
		{
#ifdef _DEBUG
			CString DLLPath = IOSrv->ResolveAssigns("Home:../DEMShaderCompiler/DEMShaderCompiler_d.dll");
#else
			CString DLLPath = IOSrv->ResolveAssigns("Home:../DEMShaderCompiler/DEMShaderCompiler.dll");
#endif
			CString DBFilePath = IOSrv->ResolveAssigns("SrcShaders:ShaderDB.db3");
			CString OutputDir = PathUtils::GetAbsolutePath(IOSrv->ResolveAssigns("Home:"), IOSrv->ResolveAssigns("Shaders:Bin/"));
			if (!InitDEMShaderCompilerDLL(DLLPath, DBFilePath, OutputDir)) FAIL;

			for (UPTR i = 0; i < ShadersDesc->GetCount(); ++i)
			{
				Data::CParam& GfxAPIPrm = ShadersDesc->Get(i);
				Data::PParams GfxAPIDesc = GfxAPIPrm.GetValue<Data::PParams>();
				for (UPTR j = 0; j < GfxAPIDesc->GetCount(); ++j)
				{
					Data::CParam& Prm = GfxAPIDesc->Get(j);
					Data::PParams ShaderSection = Prm.GetValue<Data::PParams>();

					U32 ShaderID;
					if (!ProcessShaderResourceDesc(*ShaderSection, CompileDebugShaders, false, ShaderID))
					{
						TermDEMShaderCompilerDLL();
						FAIL;
					}

					// Substitute shader desc with a compiled shader ID
					Prm.SetValue<int>(ShaderID);

					AddShaderToPack(ShaderID);

					n_msg(VL_DETAILS, "UI shader for %s: %s -> ID %d\n", GfxAPIPrm.GetName().CStr(), ShaderSection->Get<CString>(CStrID("In")).CStr(), ShaderID);
				}
			}

			if (!TermDEMShaderCompilerDLL()) FAIL;

			IOSrv->CreateDirectory(PathUtils::ExtractDirName(RealExportFilePath));
			if (!DataSrv->SavePRM(RealExportFilePath, UIDesc)) FAIL;
		}
	}

	FilesToPack.InsertSorted(RealExportFilePath);

	// If desc was not re-exported, we need to find shader references in a .prm
	if (ExportDescs)
	{
		UIDesc = DataSrv->LoadPRM(RealExportFilePath, false);

		Data::PParams ShadersDesc;
		if (UIDesc->Get<Data::PParams>(ShadersDesc, CStrID("Shaders")))
		{
			for (UPTR i = 0; i < ShadersDesc->GetCount(); ++i)
			{
				Data::PParams GfxAPIDesc = ShadersDesc->Get<Data::PParams>(i);
				for (UPTR j = 0; j < GfxAPIDesc->GetCount(); ++j)
				{
					U32 ShaderID = (U32)GfxAPIDesc->Get<int>(j);
					if (ShaderID) AddShaderToPack(ShaderID);
				}
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessDesc(const char* pSrcFilePath, const char* pExportFilePath)
{
	// Some descs can be loaded twice or more from different IO path assigns, avoid it
	CString RealExportFilePath = IOSrv->ResolveAssigns(pExportFilePath);

	if (IsFileAdded(RealExportFilePath)) OK;

	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(RealExportFilePath));
		if (!DataSrv->SavePRM(RealExportFilePath, DataSrv->LoadHRD(pSrcFilePath, false))) FAIL;
	}
	FilesToPack.InsertSorted(RealExportFilePath);

	OK;
}
//---------------------------------------------------------------------

//!!!TMP!
#include <Math/Matrix44.h>
#include <Math/TransformSRT.h>

//!!!TMP!
struct CBoneInfo
{
	matrix44	Pose;
	CStrID		BoneID;
	UPTR		ParentIndex;

	CBoneInfo(): ParentIndex(INVALID_INDEX) {}
};

//!!!TMP!
void GatherSkinInfo(CStrID NodeID, Data::PParams NodeDesc, CArray<CBoneInfo>& OutSkinInfo, UPTR ParentIndex)
{
	UPTR BoneIndex = ParentIndex;
	Data::PDataArray Attrs;
	if (NodeDesc->Get<Data::PDataArray>(Attrs, CStrID("Attrs")))
	{
		for (UPTR i = 0; i < Attrs->GetCount(); ++i)
		{
			Data::PParams AttrDesc = Attrs->Get<Data::PParams>(i);
			CString ClassName;
			if (AttrDesc->Get<CString>(ClassName, CStrID("Class")) && ClassName == "Bone")
			{
				BoneIndex = (UPTR)AttrDesc->Get<int>(CStrID("BoneIndex"));
				CBoneInfo& BoneInfo = OutSkinInfo.At(BoneIndex);

				vector4 VRotate = AttrDesc->Get<vector4>(CStrID("PoseR"), vector4(0.f, 0.f, 0.f, 1.f));
				Math::CTransformSRT SRTPoseLocal;
				SRTPoseLocal.Scale = AttrDesc->Get<vector3>(CStrID("PoseS"), vector3(1.f, 1.f, 1.f));
				SRTPoseLocal.Translation = AttrDesc->Get<vector3>(CStrID("PoseT"), vector3::Zero);
				SRTPoseLocal.Rotation.set(VRotate.x, VRotate.y, VRotate.z, VRotate.w);

				SRTPoseLocal.ToMatrix(BoneInfo.Pose);	
				if (ParentIndex != INVALID_INDEX) BoneInfo.Pose.mult_simple(OutSkinInfo[ParentIndex].Pose);

				BoneInfo.BoneID = NodeID;
				BoneInfo.ParentIndex = ParentIndex;
			}
		}
	}

	Data::PParams Children;
	if (NodeDesc->Get<Data::PParams>(Children, CStrID("Children")))
		for (UPTR i = 0; i < Children->GetCount(); ++i)
		{
			Data::CParam& Child = Children->Get(i);
			GatherSkinInfo(Child.GetName(), Child.GetValue<Data::PParams>(), OutSkinInfo, BoneIndex);
		}
}
//---------------------------------------------------------------------

//!!!TMP!
bool ConvertOldSkinInfo(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc = DataSrv->LoadHRD(SrcFilePath, false);
	if (!Desc.IsValidPtr()) FAIL;

	CArray<CBoneInfo> SkinInfo;

	GatherSkinInfo(CStrID::Empty, Desc, SkinInfo, INVALID_INDEX);

	if (SkinInfo.GetCount())
	{
		IO::PStream File = IOSrv->CreateStream(ExportFilePath);
		if (!File->Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) FAIL;
		IO::CBinaryWriter Writer(*File);

		Writer.Write<U32>('SKIF');	// Magic
		Writer.Write<U32>(1);		// Format version

		Writer.Write<U32>(SkinInfo.GetCount());

		// Write pose array
		for (UPTR i = 0; i < SkinInfo.GetCount(); ++i)
		{
			CBoneInfo& BoneInfo = SkinInfo[i];
			matrix44 Pose;
			BoneInfo.Pose.invert_simple(Pose);
			Writer.Write(Pose);
		}

		// Write bone hierarchy
		for (UPTR i = 0; i < SkinInfo.GetCount(); ++i)
		{
			CBoneInfo& BoneInfo = SkinInfo[i];
			Writer.Write<U16>(BoneInfo.ParentIndex); // Invalid index is converted correctly, all bits set
			Writer.Write(BoneInfo.BoneID);
		}

		File->Close();

		FilesToPack.InsertSorted(ExportFilePath);
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessEntity(const Data::CParams& EntityDesc)
{
	// Entity tpls aren't exported by ref here, because ALL that tpls are exported.
	// This allows to instantiate unreferenced tpls at the runtime.

	Data::PParams Attrs;
	if (!EntityDesc.Get<Data::PParams>(Attrs, CStrID("Attrs"))) OK;

	CString AttrValue;

	if (Attrs->Get<CString>(AttrValue, CStrID("UIDesc")))
		if (!ProcessDesc("SrcGameUI:" + AttrValue + ".hrd", "GameUI:" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing UI desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("ActorDesc")))
	{
		if (!ProcessDescWithParents(CString("SrcActors:"), CString("Actors:"), AttrValue))
		{
			n_msg(VL_ERROR, "Error processing AI actor desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

		if (!ProcessDesc("SrcAI:AIActionTpls.hrd", "AI:AIActionTpls.prm"))
		{
			n_msg(VL_ERROR, "Error processing shared AI action templates desc\n");
			FAIL;
		}
	}

	if (Attrs->Get<CString>(AttrValue, CStrID("AIHintsDesc")))
		if (!ProcessDesc("SrcAIHints:" + AttrValue + ".hrd", "AIHints:" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing AI hints desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("SODesc")))
	{
		if (!ProcessDesc("SrcSmarts:" + AttrValue + ".hrd", "Smarts:" + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing AI smart object desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

		if (!ProcessSOActionTplsDesc(CString("SrcAI:AISOActionTpls.hrd"), CString("AI:AISOActionTpls.prm")))
		{
			n_msg(VL_ERROR, "Error processing shared smart object action templates desc\n");
			FAIL;
		}
	}

	CStrID ItemID = Attrs->Get<CStrID>(CStrID("ItemTplID"), CStrID::Empty);
	if (ItemID.IsValid())
		if (!ProcessDesc(CString("SrcItems:") + ItemID.CStr() + ".hrd", CString("Items:") + ItemID.CStr() + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing item desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	Data::PDataArray Inventory;
	if (Attrs->Get<Data::PDataArray>(Inventory, CStrID("Inventory")))
	{
		for (UPTR i = 0; i < Inventory->GetCount(); ++i)
		{
			ItemID = Inventory->Get<Data::PParams>(i)->Get<CStrID>(CStrID("ID"), CStrID::Empty);
			if (ItemID.IsValid() &&
				!ProcessDesc(	CString("SrcItems:") + ItemID.CStr() + ".hrd",
								CString("Items:") + ItemID.CStr() + ".prm"))
			{
				n_msg(VL_ERROR, "Error processing item desc '%s'\n", AttrValue.CStr());
				FAIL;
			}
		}
	}

	if (Attrs->Get<CString>(AttrValue, CStrID("ScriptClass")))
	{
		CString ExportFilePath = "ScriptClasses:" + AttrValue + ".cls";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "SrcScriptClasses:" + AttrValue + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	if (Attrs->Get<CString>(AttrValue, CStrID("Script")))
	{
		CString ExportFilePath = "Scripts:" + AttrValue + ".lua";
		if (!IsFileAdded(ExportFilePath))
		{
			if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "SrcScripts:" + AttrValue + ".lua", ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}

	CStrID PickShape = Attrs->Get<CStrID>(CStrID("PickShape"), CStrID::Empty);
	if (PickShape.IsValid())
		if (!ProcessCollisionShape(	CString("SrcPhysics:") + PickShape.CStr() + ".hrd",
									CString("Physics:") + PickShape.CStr() + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing collision shape '%s'\n", PickShape.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("Physics")))
		if (!ProcessPhysicsDesc(CString("SrcPhysics:") + AttrValue + ".hrd",
								CString("Physics:") + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing physics desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("AnimDesc")))
		if (!ProcessAnimDesc(	CString("SrcGameAnim:") + AttrValue + ".hrd",
								CString("GameAnim:") + AttrValue + ".prm"))
		{
			n_msg(VL_ERROR, "Error processing animation desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("Dialogue")))
		if (!ProcessDialogue(CString("SrcDlg:"), CString("Dlg:"), AttrValue))
		{
			n_msg(VL_ERROR, "Error processing dialogue desc '%s'\n", AttrValue.CStr());
			FAIL;
		}

	if (Attrs->Get<CString>(AttrValue, CStrID("SceneFile")))
	{
		if (!ProcessSceneResource("SrcScene:" + AttrValue + ".hrd", "Scene:" + AttrValue + ".scn"))
		{
			n_msg(VL_ERROR, "Error processing scene resource '%s'\n", AttrValue.CStr());
			FAIL;
		}

		//!!!TMP! Convert old skin info to new
		if (!ConvertOldSkinInfo("SrcScene:" + AttrValue + ".hrd", "Scene:" + AttrValue + ".skn"))
		{
			n_msg(VL_ERROR, "Error processing scene resource skin info '%s'\n", AttrValue.CStr());
			FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessLevel(const Data::CParams& LevelDesc, const CString& Name)
{
	// Add level script

	CString ExportFilePath = "Levels:" + Name + ".lua";
	if (ExportDescs)
	{
		CString SrcFilePath = "SrcLevels:" + Name + ".lua";
		if (!IsFileAdded(SrcFilePath) && IOSrv->FileExists(SrcFilePath))
		{
			BatchToolInOut(CStrID("CFLua"), SrcFilePath, ExportFilePath);
			FilesToPack.InsertSorted(ExportFilePath);
		}
	}
	else if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);

	// Add navmesh (always exported, no src)

	ExportFilePath = "Levels:" + Name + ".nm";
	if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);

	// Export entities

	Data::PParams SubDesc;
	if (LevelDesc.Get(SubDesc, CStrID("Entities")))
	{
		for (UPTR i = 0; i < SubDesc->GetCount(); ++i)
		{
			const Data::CParam& EntityPrm = SubDesc->Get(i);
			if (!EntityPrm.IsA<Data::PParams>()) continue;
			Data::PParams EntityDesc = EntityPrm.GetValue<Data::PParams>();
			n_msg(VL_INFO, " Processing entity '%s'...\n", EntityPrm.GetName().CStr());
			if (!EntityDesc.IsValidPtr() || !ProcessEntity(*EntityDesc))
			{
				n_msg(VL_ERROR, "Error processing entity '%s'\n", EntityPrm.GetName().CStr());
				FAIL;
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool ProcessDescsInFolder(const CString& SrcPath, const CString& ExportPath)
{
	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(ExportDescs ? SrcPath : ExportPath))
	{
		n_msg(VL_ERROR, "Could not open directory '%s' for reading!\n", Browser.GetCurrentPath().CStr());
		FAIL;
	}

	if (ExportDescs) IOSrv->CreateDirectory(ExportPath);

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			CString DescName = PathUtils::ExtractFileNameWithoutExtension(Browser.GetCurrEntryName());

			n_msg(VL_INFO, "Processing desc '%s'...\n", DescName.CStr());

			CString ExportFilePath = ExportPath + "/" + DescName + ".prm";
			if (!ProcessDesc(Browser.GetCurrentPath() + "/" + Browser.GetCurrEntryName(), ExportFilePath))
			{
				n_msg(VL_ERROR, "Error loading desc '%s'\n", DescName.CStr());
				continue;
			}
		}
		else if (Browser.IsCurrEntryDir())
		{
			if (!ProcessDescsInFolder(SrcPath + "/" + Browser.GetCurrEntryName(), ExportPath + "/" + Browser.GetCurrEntryName())) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

bool ProcessEntityTplsInFolder(const CString& SrcPath, const CString& ExportPath)
{
	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(ExportDescs ? SrcPath : ExportPath))
	{
		n_msg(VL_ERROR, "Could not open directory '%s' for reading!\n", Browser.GetCurrentPath().CStr());
		FAIL;
	}

	if (ExportDescs) IOSrv->CreateDirectory(ExportPath);

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			CString DescName = PathUtils::ExtractFileNameWithoutExtension(Browser.GetCurrEntryName());

			n_msg(VL_INFO, "Processing entity tpl '%s'...\n", DescName.CStr());

			CString SrcFilePath = Browser.GetCurrentPath() + "/" + Browser.GetCurrEntryName();
			CString RealExportFilePath = IOSrv->ResolveAssigns(ExportPath + "/" + DescName + ".prm");

			if (IsFileAdded(RealExportFilePath)) continue;

			if (ExportDescs)
			{
				Data::PParams Desc = DataSrv->LoadHRD(SrcFilePath, false);
				if (!Desc.IsValidPtr())
				{
					n_msg(VL_ERROR, "Error loading entity tpl '%s'\n", DescName.CStr());
					FAIL;
				}

				Data::PDataArray Props;
				if (Desc->Get<Data::PDataArray>(Props, CStrID("Props")) && Props->GetCount())
					ConvertPropNamesToFourCC(Props);

				IOSrv->CreateDirectory(PathUtils::ExtractDirName(RealExportFilePath));
				if (!DataSrv->SavePRM(RealExportFilePath, Desc))
				{
					n_msg(VL_ERROR, "Error saving entity tpl '%s'\n", DescName.CStr());
					FAIL;
				}

				if (!ProcessEntity(*Desc))
				{
					n_msg(VL_ERROR, "Error processing entity tpl '%s'\n", DescName.CStr());
					FAIL;
				}
			}
			FilesToPack.InsertSorted(RealExportFilePath);
		}
		else if (Browser.IsCurrEntryDir())
		{
			if (!ProcessEntityTplsInFolder(SrcPath + "/" + Browser.GetCurrEntryName(), ExportPath + "/" + Browser.GetCurrEntryName())) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

bool ProcessQuestsInFolder(const CString& SrcPath, const CString& ExportPath)
{
	IO::CFSBrowser Browser;
	if (!Browser.SetAbsolutePath(ExportDescs ? SrcPath : ExportPath))
	{
		n_msg(VL_ERROR, "Could not open directory '%s' for reading!\n", Browser.GetCurrentPath().CStr());
		FAIL;
	}

	if (ExportDescs) IOSrv->CreateDirectory(ExportPath);

	if (!Browser.IsCurrDirEmpty()) do
	{
		if (Browser.IsCurrEntryFile())
		{
			CString LowerName = Browser.GetCurrEntryName();
			LowerName.ToLower();

			if (LowerName != (ExportDescs ? "_quest.hrd" : "_quest.prm")) continue;

			CString QuestName;
			if (Verbose >= VL_INFO)
			{
				QuestName = Browser.GetCurrentPath();
				QuestName.Trim(" \r\n\t\\/", false);
				QuestName = PathUtils::ExtractFileName(QuestName);
			}

			n_msg(VL_INFO, "Processing quest '%s'...\n", QuestName.CStr());

			CString ExportFilePath = ExportPath + "/_Quest.prm";
			Data::PParams Desc;
			if (ExportDescs)
			{
				Desc = DataSrv->LoadHRD(SrcPath + "/_Quest.hrd", false);
				DataSrv->SavePRM(ExportFilePath, Desc);
			}
			else Desc = DataSrv->LoadPRM(ExportFilePath, false);

			if (!Desc.IsValidPtr())
			{
				n_msg(VL_ERROR, "Error loading quest '%s' desc\n", QuestName.CStr());
				continue;
			}

			FilesToPack.InsertSorted(ExportFilePath);

			Data::PParams Tasks;
			if (Desc->Get(Tasks, CStrID("Tasks")))
			{
				for (UPTR i = 0; i < Tasks->GetCount(); ++i)
				{
					CString Name(Tasks->Get(i).GetName().CStr());
					ExportFilePath = ExportPath + "/" + Name + ".lua";
					if (ExportDescs)
					{
						CString SrcFilePath = SrcPath + "/" + Name + ".lua";
						if (!IsFileAdded(SrcFilePath) && IOSrv->FileExists(SrcFilePath))
						{
							BatchToolInOut(CStrID("CFLua"), SrcFilePath, ExportFilePath);
							FilesToPack.InsertSorted(ExportFilePath);
						}
					}
					else if (!IsFileAdded(ExportFilePath) && IOSrv->FileExists(ExportFilePath)) FilesToPack.InsertSorted(ExportFilePath);
				}
			}
		}
		else if (Browser.IsCurrEntryDir())
		{
			if (!ProcessQuestsInFolder(SrcPath + "/" + Browser.GetCurrEntryName(), ExportPath + "/" + Browser.GetCurrEntryName())) FAIL;
		}
	}
	while (Browser.NextCurrDirEntry());

	OK;
}
//---------------------------------------------------------------------

bool ProcessSOActionTplsDesc(const CString& SrcFilePath, const CString& ExportFilePath)
{
	if (IsFileAdded(ExportFilePath)) OK;

	Data::PParams Desc;
	if (ExportDescs)
	{
		IOSrv->CreateDirectory(PathUtils::ExtractDirName(ExportFilePath));
		Desc = DataSrv->LoadHRD(SrcFilePath, false);
		if (!DataSrv->SavePRM(ExportFilePath, Desc)) FAIL;
	}
	else Desc = DataSrv->LoadPRM(ExportFilePath);

	if (!Desc.IsValidPtr()) FAIL;

	FilesToPack.InsertSorted(ExportFilePath);

	for (UPTR i = 0; i < Desc->GetCount(); ++i)
	{
		const Data::CParams& ActTpl = *Desc->Get<Data::PParams>(i);

		CString ScriptFile;
		if (ActTpl.Get<CString>(ScriptFile, CStrID("Script")))
		{
			CString ExportFilePath = "Scripts:" + ScriptFile + ".lua";
			if (!IsFileAdded(ExportFilePath))
			{
				if (ExportDescs) BatchToolInOut(CStrID("CFLua"), "SrcScripts:" + ScriptFile + ".lua", ExportFilePath);
				FilesToPack.InsertSorted(ExportFilePath);
			}
		}
	}

	OK;
}
//---------------------------------------------------------------------
