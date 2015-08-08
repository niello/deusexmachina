#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/PathUtils.h>
#include <Data/Buffer.h>
#include <Data/Params.h>
#include <Data/DataArray.h>
#include <Data/HRDParser.h>
#include <ToolRenderStateDesc.h>
#include <ConsoleApp.h>
#include <DEMD3DInclude.h>
#include <D3DCompiler.h>

#undef CreateDirectory

extern CString RootPath;

//!!!compile old sm3.0 shaders for DX9!
int CompileShader(const char* pInFilePath, const char* pOutFilePath, bool Debug)
{
	Data::CBuffer In;
	if (!IOSrv->LoadFileToBuffer(pInFilePath, In)) return ERR_IO_READ;

	//D3DCompile
	//D3DCompile2
	//D3DPreprocess - apply preprocessor to text
	//D3DGetBlobPart, D3D_BLOB_INPUT_SIGNATURE_BLOB
	//D3DStripShader - remove blobs
	//D3DCompressShaders
	//D3DDecompressShaders

//D3DCOMPILE_IEEE_STRICTNESS
//#define D3DCOMPILE_IEEE_STRICTNESS                (1 << 13)
//#define D3DCOMPILE_OPTIMIZATION_LEVEL0            (1 << 14) // fast creation, lowest opt
//#define D3DCOMPILE_OPTIMIZATION_LEVEL1            0
//#define D3DCOMPILE_OPTIMIZATION_LEVEL2            ((1 << 14) | (1 << 15))
//#define D3DCOMPILE_OPTIMIZATION_LEVEL3            (1 << 15) // final opt
	//D3DCOMPILE_WARNINGS_ARE_ERRORS
	// D3DCOMPILE_DEBUG - debug info
	// D3DCOMPILE_SKIP_OPTIMIZATION - only for active debug
	// D3DCOMPILE_SKIP_VALIDATION - faster, if successfully compiled
	// D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR (more efficient, vec*mtx dots)
	// D3DCOMPILE_AVOID_FLOW_CONTROL, D3DCOMPILE_PREFER_FLOW_CONTROL
	// D3DCOMPILE_ENABLE_STRICTNESS - kill deprecated syntax
	//???do col-major matrices make GPU constants setting harder and slower?
	DWORD Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS;
	if (Debug)
	{
		Flags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
		n_msg(VL_DEBUG, "Debug compilation on\n");
	}
//ds_5_0	Domain shader
//gs_5_0	Geometry shader
//hs_5_0	Hull shader
//ps_5_0	Pixel shader
//vs_5_0	Vertex shader
//gs_4_1	Geometry shader
//ps_4_1	Pixel shader
//vs_4_1	Vertex shader
//gs_4_0	Geometry shader
//ps_4_0	Pixel shader
//vs_4_0	Vertex shader
//ps_4_0_level_9_1
//ps_4_0_level_9_3
//vs_4_0_level_9_1
//vs_4_0_level_9_3
//ps_3_0	Pixel shader 3.0 
//vs_3_0	Vertex shader 3.0

	D3D_SHADER_MACRO Defines[] = { "zero", "0", NULL, NULL };

	//???create once, in main, pass by param?
	CDEMD3DInclude IncHandler(PathUtils::ExtractDirName(pInFilePath), RootPath);

	//!!!DBG TMP!
	const char* pEntryPoint = NULL;
	const char* pTarget = NULL;

	ID3DBlob* pCode = NULL;
	ID3DBlob* pErrors = NULL;
	HRESULT hr = D3DCompile(In.GetPtr(), In.GetSize(), pInFilePath, Defines, &IncHandler, pEntryPoint, pTarget, Flags, 0, &pCode, &pErrors);

	if (FAILED(hr) || !pCode)
	{
		n_msg(VL_ERROR, "Failed to compile '%s' with:\n\n%s\n",
			pOutFilePath,
			pErrors ? pErrors->GetBufferPointer() : "No D3D error message.");
		if (pCode) pCode->Release();
		if (pErrors) pErrors->Release();
		return ERR_MAIN_FAILED;
	}
	else if (pErrors)
	{
		n_msg(VL_WARNING, "'%s' compiled with:\n\n%s\n", pOutFilePath, pErrors->GetBufferPointer());
		pErrors->Release();
	}

//!!!strip out reflection and debug data for release builds!

	IOSrv->CreateDirectory(PathUtils::ExtractDirName(pOutFilePath));

	bool Written = false;
	IO::CFileStream File;
	if (File.Open(pOutFilePath, IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		Written = File.Write(pCode->GetBufferPointer(), pCode->GetBufferSize()) == pCode->GetBufferSize();

	pCode->Release();

	return Written ? SUCCESS : ERR_IO_WRITE;
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
	if (Str == "zero") return Render::StencilOp_Replace;
	if (Str == "zero") return Render::StencilOp_Inc;
	if (Str == "zero") return Render::StencilOp_IncSat;
	if (Str == "zero") return Render::StencilOp_Dec;
	if (Str == "zero") return Render::StencilOp_DecSat;
	if (Str == "zero") return Render::StencilOp_Invert;
	return Render::StencilOp_Keep;
}
//---------------------------------------------------------------------

bool ReadRenderStateDesc(Data::PParams RenderStates, CStrID ID, Render::CToolRenderStateDesc& Desc)
{
	Data::PParams RS;
	if (!RenderStates->Get(RS, ID)) FAIL;

	Data::CParam* pPrmBaseID;
	if (RS->Get(pPrmBaseID, CStrID("Base")))
	{
		if (!ReadRenderStateDesc(RenderStates, pPrmBaseID->GetValue<CStrID>(), Desc)) FAIL;
	}

	//!!!shader refs must be converted to output shaders!
	//store src here, replace with export after compilation outside this method
	//???or compile here and store variants in cache?
	//!!!each variation of the same HLSL will be a separate compiled shader!
	//or here use compiled shader IDs and separate section Shaders where all defines and targets specified?

	RS->Get(Desc.VertexShader, CStrID("VertexShader"));
	RS->Get(Desc.PixelShader, CStrID("PixelShader"));
	RS->Get(Desc.GeometryShader, CStrID("GeometryShader"));
	RS->Get(Desc.HullShader, CStrID("HullShader"));
	RS->Get(Desc.DomainShader, CStrID("DomainShader"));

	CString StrValue;
	int IntValue;
	//float FloatValue;
	bool FlagValue;

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
	//if (RS->Get(FlagValue, CStrID("IndependentBlendEnable")))
	//	Desc.Flags.SetTo(Render::CToolRenderStateDesc::Blend_Independent, FlagValue);
	//Blend_RTBlendEnable			= 0x00040000	// Use (Blend_RTBlendEnable << Index), Index = [0 .. 7]
	//// flags from				  0x00040000
	////       to					  0x02000000
	//// inclusive are reserved for Blend_RTBlendEnable

	//BlendFactorRGBA[0] = 0.f;
	//BlendFactorRGBA[1] = 0.f;
	//BlendFactorRGBA[2] = 0.f;
	//BlendFactorRGBA[3] = 0.f;
	//SampleMask = 0xffffffff;

	//for (int i = 0; i < 8; ++i)
	//{
	//	CRTBlend& RTB = RTBlend[i];
	//	RTB.SrcBlendArg = BlendArg_One;
	//	RTB.DestBlendArg = BlendArg_Zero;
	//	RTB.BlendOp = BlendOp_Add;
	//	RTB.SrcBlendArgAlpha = BlendArg_One;
	//	RTB.DestBlendArgAlpha = BlendArg_Zero;
	//	RTB.BlendOpAlpha = BlendOp_Add;
	//	RTB.WriteMask = 0x0f;
	//}

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

	for (int i = 0; i < UsedRenderStates.GetCount(); ++i)
	{
		CStrID ID = UsedRenderStates[i];
		Render::CToolRenderStateDesc Desc;
		Desc.SetDefaults();
		if (!ReadRenderStateDesc(RenderStates, ID, Desc)) return ERR_INVALID_DATA;
		int dbg = 0;
		// Save desc under ID
		// Store shader pathes
	}

	//???Samplers? descs and per-tech register assignmemts? or per-whole-effect?
	// RenderStates (hierarchy) -> save with scheme, strings to enum codes
	// VS - get input blob, check if exists, else save with signature
	// All shaders - register mappings
	// Unwind hierarchy here, store value-only whole RS descs
	// read ony referenced states, load bases on demand once and cache

	return SUCCESS;
}
//---------------------------------------------------------------------
