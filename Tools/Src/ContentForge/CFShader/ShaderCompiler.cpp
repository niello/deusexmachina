#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/PathUtils.h>
#include <IO/BinaryWriter.h>
#include <Data/Buffer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>
#include <Data/StringTokenizer.h>
#include <Util/UtilFwd.h> // CRC
#include <ToolRenderStateDesc.h>
#include <ConsoleApp.h>
#include <ShaderDB.h>
#include <DEMD3DInclude.h>
#include <D3DCompiler.h>

#undef CreateDirectory
#undef DeleteFile

extern CString RootPath;

//???maybe pack shaders to some 'DB' file which maps DB ID to offset
//and reference shaders by DB ID, not by a file name?
//can even order shaders in memory and load one-by-one to minimize seek time
//may pack by use (common, menu, game, cinematic etc) and load only used.
//can store debug and release binary packages and read from what user wants

int CompileShader(CShaderDBRec& Rec, bool Debug)
{
	CString SrcPath = RootPath + Rec.SrcFile.Path;
	Data::CBuffer In;
	if (!IOSrv->LoadFileToBuffer(SrcPath, In)) return ERR_IO_READ;
	DWORD CurrWriteTime = IOSrv->GetFileWriteTime(SrcPath);
	DWORD SrcCRC = Util::CalcCRC((uchar*)In.GetPtr(), In.GetSize());

	// Setup compiler flags

	// D3DCOMPILE_IEEE_STRICTNESS
	// D3DCOMPILE_AVOID_FLOW_CONTROL, D3DCOMPILE_PREFER_FLOW_CONTROL

	DWORD Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR; // (more efficient, vec*mtx dots) //???does touch CPU-side const binding code?
	if (Rec.Target >= 0x0400)
	{
		Flags |= D3DCOMPILE_ENABLE_STRICTNESS; // Denies deprecated syntax
	}
	else
	{
		Flags |= D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY;
	}
	if (Debug)
	{
		Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
		n_msg(VL_DEBUG, "Debug compilation on\n");
	}
	else Flags |= (D3DCOMPILE_OPTIMIZATION_LEVEL3); // | D3DCOMPILE_SKIP_VALIDATION);

	// Get info about previous compilation, skip if no changes

	Rec.SrcFile.Path = SrcPath;

	bool RecFound = FindShaderRec(Rec);
	if (RecFound &&
		Rec.CompilerFlags == Flags &&
		Rec.SrcFile.Size == In.GetSize() &&
		Rec.SrcFile.CRC == SrcCRC &&
		CurrWriteTime &&
		Rec.SrcModifyTimestamp == CurrWriteTime)
	{
		return SUCCESS;
	}

	Rec.CompilerFlags = Flags;
	Rec.SrcFile.Size = In.GetSize();
	Rec.SrcFile.CRC = SrcCRC;
	Rec.SrcModifyTimestamp = CurrWriteTime;

	// Determine D3D target

	const char* pTarget = NULL;
	switch (Rec.ShaderType)
	{
		case Render::ShaderType_Vertex:
		{
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "vs_5_0"; break;
				case 0x0401: pTarget = "vs_4_1"; break;
				case 0x0400: pTarget = "vs_4_0"; break;
				case 0x0300: pTarget = "vs_3_0"; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Pixel:
		{
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "ps_5_0"; break;
				case 0x0401: pTarget = "ps_4_1"; break;
				case 0x0400: pTarget = "ps_4_0"; break;
				case 0x0300: pTarget = "ps_3_0"; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Geometry:
		{
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "gs_5_0"; break;
				case 0x0401: pTarget = "gs_4_1"; break;
				case 0x0400: pTarget = "gs_4_0"; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Hull:
		{
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "hs_5_0"; break;
				default: FAIL;
			}
			break;
		}
		case Render::ShaderType_Domain:
		{
			switch (Rec.Target)
			{
				case 0x0500: pTarget = "ds_5_0"; break;
				default: FAIL;
			}
			break;
		}
		default: FAIL;
	};

	// Setup shader macros

	CArray<D3D_SHADER_MACRO> Defines(8, 4);
	D3D_SHADER_MACRO* pDefines;
	if (Rec.Defines.GetCount())
	{
		for (int i = 0; i < Rec.Defines.GetCount(); ++i)
		{
			const CMacroDBRec& Macro = Rec.Defines[i];
			D3D_SHADER_MACRO D3DMacro = { Macro.Name, Macro.Value };
			Defines.Add(D3DMacro);
		}

		pDefines = &Defines[0];
	}
	else pDefines = NULL;

	// Compile shader

	CDEMD3DInclude IncHandler(PathUtils::ExtractDirName(Rec.SrcFile.Path), RootPath);

	ID3DBlob* pCode = NULL;
	ID3DBlob* pErrors = NULL;
	HRESULT hr = D3DCompile(In.GetPtr(), In.GetSize(), Rec.SrcFile.Path.CStr(),
							pDefines, &IncHandler, Rec.EntryPoint.CStr(), pTarget,
							Flags, 0, &pCode, &pErrors);

	if (FAILED(hr) || !pCode)
	{
		n_msg(VL_ERROR, "Failed to compile '%s' with:\n\n%s\n",
			Rec.ObjFile.Path.CStr(),
			pErrors ? pErrors->GetBufferPointer() : "No D3D error message.");
		if (pCode) pCode->Release();
		if (pErrors) pErrors->Release();
		return ERR_MAIN_FAILED;
	}
	else if (pErrors)
	{
		n_msg(VL_WARNING, "'%s' compiled with warnings:\n\n%s\n", Rec.ObjFile.Path.CStr(), pErrors->GetBufferPointer());
		pErrors->Release();
	}

	// Strip unnecessary info for release builds, making object files smaller

	ID3DBlob* pFinalCode;
	if (Debug) pFinalCode = pCode;
	else
	{
		hr = D3DStripShader(pCode->GetBufferPointer(),
							pCode->GetBufferSize(),
							D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS | D3DCOMPILER_STRIP_PRIVATE_DATA,
							&pFinalCode);
		pCode->Release();
		if (FAILED(hr)) return ERR_MAIN_FAILED;
	}

	Rec.ObjFile.Size = pFinalCode->GetBufferSize();
	Rec.ObjFile.CRC = Util::CalcCRC((uchar*)pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());

	// Try to find exactly the same binary and reuse it, or save our result

	DWORD OldObjFileID = Rec.ObjFile.ID;
	bool ObjFound = FindObjFile(Rec.ObjFile, pFinalCode->GetBufferPointer());
	if (!ObjFound)
	{
		if (!RegisterObjFile(Rec.ObjFile, "cso")) // Fills empty ID and path inside
		{
			pFinalCode->Release();
			return ERR_MAIN_FAILED;
		}

		IOSrv->CreateDirectory(PathUtils::ExtractDirName(Rec.ObjFile.Path));

		IO::CFileStream File;
		if (!File.Open(Rec.ObjFile.Path, IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		{
			pFinalCode->Release();
			return ERR_MAIN_FAILED;
		}
		File.Write(pFinalCode->GetBufferPointer(), pFinalCode->GetBufferSize());
		File.Close();
	}

	if (OldObjFileID > 0 && OldObjFileID != Rec.ObjFile.ID)
	{
		CString OldObjPath;
		if (ReleaseObjFile(OldObjFileID, OldObjPath))
			IOSrv->DeleteFile(OldObjPath);
	}

	// For vertex shaders, store input signature separately.
	// It saves RAM since input signatures must reside there as a raw data.

	if (Rec.ShaderType == Render::ShaderType_Vertex)
	{
		ID3DBlob* pInputSig;
		if (SUCCEEDED(D3DGetBlobPart(pFinalCode->GetBufferPointer(),
									 pFinalCode->GetBufferSize(),
									 D3D_BLOB_INPUT_SIGNATURE_BLOB,
									 0,
									 &pInputSig)))
		{
			pFinalCode->Release();

			Rec.InputSigFile.Size = pInputSig->GetBufferSize();
			Rec.InputSigFile.CRC = Util::CalcCRC((uchar*)pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());

			DWORD OldInputSigID = Rec.InputSigFile.ID;
			bool ObjFound = FindObjFile(Rec.InputSigFile, pInputSig->GetBufferPointer());
			if (!ObjFound)
			{
				if (!RegisterObjFile(Rec.InputSigFile, "sig")) // Fills empty ID and path inside
				{
					pInputSig->Release();
					return ERR_MAIN_FAILED;
				}

				IOSrv->CreateDirectory(PathUtils::ExtractDirName(Rec.InputSigFile.Path));

				IO::CFileStream File;
				if (!File.Open(Rec.InputSigFile.Path, IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
				{
					pInputSig->Release();
					return ERR_MAIN_FAILED;
				}
				File.Write(pInputSig->GetBufferPointer(), pInputSig->GetBufferSize());
				File.Close();
			}

			if (OldInputSigID > 0 && OldInputSigID != Rec.InputSigFile.ID)
			{
				CString OldObjPath;
				if (ReleaseObjFile(OldInputSigID, OldObjPath))
					IOSrv->DeleteFile(OldObjPath);
			}

			pInputSig->Release();
		}
		else
		{
			Rec.InputSigFile.ID = Rec.ObjFile.ID;
			pFinalCode->Release();
		}
	}
	else pFinalCode->Release();

	return WriteShaderRec(Rec) ? SUCCESS : ERR_MAIN_FAILED;
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

bool ProcessShaderSection(Data::PParams RenderState, CStrID SectionID, Render::EShaderType ShaderType, bool Debug, CString& OutShaderExportPath)
{
	Data::PParams ShaderSection;
	if (!RenderState->Get(ShaderSection, SectionID)) OK;

	int IntValue;

	CShaderDBRec Rec;
	Rec.ShaderType = ShaderType;
	ShaderSection->Get(Rec.SrcFile.Path, CStrID("Src"));
	ShaderSection->Get(Rec.EntryPoint, CStrID("Entry"));
	if (ShaderSection->Get(IntValue, CStrID("Target"))) Rec.Target = IntValue;

	CString Defines;
	if (ShaderSection->Get(Defines, CStrID("Defines"))) // NAME[=VALUE];NAME[=VALUE];...NAME[=VALUE]
	{
		Defines.Trim();

		if (Defines.GetLength())
		{
			Rec.pDefineString = (char*)n_malloc(Defines.GetLength() + 1);
			strcpy_s(Rec.pDefineString, Defines.GetLength() + 1, Defines.CStr());

			CMacroDBRec CurrMacro = { 0 };

			char* pCurrPos = Rec.pDefineString;
			const char* pBothDlms = "=;";
			const char* pSemicolonOnly = ";";
			const char* pCurrDlms = pBothDlms;
			while (true)
			{
				char* pDlm = strpbrk(pCurrPos, pCurrDlms);
				if (pDlm)
				{
					char Dlm = *pDlm;
					if (Dlm == '=')
					{
						CurrMacro.Name = pCurrPos;
						CurrMacro.Value = pDlm + 1;
						Rec.Defines.Add(CurrMacro);
						pCurrDlms = pSemicolonOnly;
					}
					else // ';'
					{
						CurrMacro.Value = NULL;
						if (!CurrMacro.Name)
						{
							CurrMacro.Name = pCurrPos;
							Rec.Defines.Add(CurrMacro);
						}
						CurrMacro.Name = NULL;
						pCurrDlms = pBothDlms;
					}
					*pDlm = 0;
					pCurrPos = pDlm + 1;
				}
				else
				{
					CurrMacro.Value = NULL;
					if (!CurrMacro.Name)
					{
						CurrMacro.Name = pCurrPos;
						Rec.Defines.Add(CurrMacro);
					}
					CurrMacro.Name = NULL;
					break;
				}
			}

			Rec.Defines.Add(CurrMacro); // Both NULLs in all control pathes
		}
	}

	Rec.SrcFile.Path = IOSrv->ResolveAssigns(Rec.SrcFile.Path);
	Rec.SrcFile.Path.Replace(RootPath, "");

	return (CompileShader(Rec, Debug) == SUCCESS);
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

bool ReadRenderStateDesc(Data::PParams RenderStates, CStrID ID, Render::CToolRenderStateDesc& Desc, bool Debug, bool Leaf)
{
	Data::PParams RS;
	if (!RenderStates->Get(RS, ID)) FAIL;

	Data::CParam* pPrmBaseID;
	if (RS->Get(pPrmBaseID, CStrID("Base")))
	{
		if (!ReadRenderStateDesc(RenderStates, pPrmBaseID->GetValue<CStrID>(), Desc, Debug, false)) FAIL;
	}

	//for that need to store defaults in a blend desc
	//if (Leaf) //!!!merge shader descs (partial in base, final in a leaf)
	//{
		if (!ProcessShaderSection(RS, CStrID("VS"), Render::ShaderType_Vertex, Debug, Desc.VertexShader)) FAIL;
		if (!ProcessShaderSection(RS, CStrID("PS"), Render::ShaderType_Pixel, Debug, Desc.PixelShader)) FAIL;
		if (!ProcessShaderSection(RS, CStrID("GS"), Render::ShaderType_Geometry, Debug, Desc.GeometryShader)) FAIL;
		if (!ProcessShaderSection(RS, CStrID("HS"), Render::ShaderType_Hull, Debug, Desc.HullShader)) FAIL;
		if (!ProcessShaderSection(RS, CStrID("DS"), Render::ShaderType_Domain, Debug, Desc.DomainShader)) FAIL;
	//}

	CString StrValue;
	int IntValue;
	//float FloatValue;
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
			for (int BlendIdx = 0; BlendIdx < BlendArray->GetCount(); ++BlendIdx)
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

	//Misc_ClipPlaneEnable		= 0x08000000 //!!!need 6 bits! or uchar/DWORD w/lower 6 bits

	OK;
}
//---------------------------------------------------------------------

int CompileEffect(const char* pInFilePath, const char* pOutFilePath, bool Debug)
{
	Data::CBuffer Buffer;
	if (!IOSrv->LoadFileToBuffer(pInFilePath, Buffer)) return ERR_IO_READ;

	Data::PParams Params;
	{
		Data::CHRDParser Parser;
		if (!Parser.ParseBuffer((LPCSTR)Buffer.GetPtr(), Buffer.GetSize(), Params)) return ERR_IO_READ;
	}

	Data::PParams Techs;
	Data::PParams RenderStates;
	if (!Params->Get(Techs, CStrID("Techniques"))) return ERR_INVALID_DATA;
	if (!Params->Get(RenderStates, CStrID("RenderStates"))) return ERR_INVALID_DATA;

	// Collect used render state references

	CArray<CStrID> UsedRenderStates;
	for (int TechIdx = 0; TechIdx < Techs->GetCount(); ++TechIdx)
	{
		Data::CParam& Tech = Techs->Get(TechIdx);
		Data::PDataArray Passes;
		if (!Tech.GetValue<Data::PParams>()->Get(Passes, CStrID("Passes"))) continue;

		for (int PassIdx = 0; PassIdx < Passes->GetCount(); ++PassIdx)
		{
			CStrID PassID = Passes->Get<CStrID>(PassIdx);
			n_msg(VL_DETAILS, "Tech %s, Pass %d: %s\n", Tech.GetName().CStr(), PassIdx, PassID.CStr());
			if (!UsedRenderStates.Contains(PassID)) UsedRenderStates.Add(PassID);
		}
	}

	// Unwind render state hierarchy and save leaf states

	IOSrv->CreateDirectory(PathUtils::ExtractDirName(pOutFilePath));
	
	IO::CFileStream File;
	if (!File.Open(pOutFilePath, IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) return ERR_IO_WRITE;
	IO::CBinaryWriter W(File);

	W.Write('SHFX');
	W.Write(0x0100);
	W.Write(UsedRenderStates.GetCount());

	for (int i = 0; i < UsedRenderStates.GetCount(); ++i)
	{
		CStrID ID = UsedRenderStates[i];
		Render::CToolRenderStateDesc Desc;
		Desc.SetDefaults();
		if (!ReadRenderStateDesc(RenderStates, ID, Desc, Debug, true)) return ERR_INVALID_DATA;

		if (!W.Write(ID)) return ERR_IO_WRITE;
		// Save desc under ID
		// Store shader pathes
	}

	File.Close();

	//???Samplers? descs and per-tech register assignmemts? or per-whole-effect?
	// RenderStates (hierarchy) -> save with scheme, strings to enum codes
	// VS - get input blob, check if exists, else save with signature
	// All shaders - register mappings
	// Unwind hierarchy here, store value-only whole RS descs
	// read ony referenced states, load bases on demand once and cache

	return SUCCESS;
}
//---------------------------------------------------------------------
