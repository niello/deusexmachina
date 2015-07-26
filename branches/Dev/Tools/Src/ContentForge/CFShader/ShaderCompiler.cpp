#include <IO/IOServer.h>
#include <IO/Streams/FileStream.h>
#include <IO/PathUtils.h>
#include <Data/Buffer.h>
#include <Data/HRDParser.h>
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

	IOSrv->CreateDirectory(PathUtils::ExtractDirName(pOutFilePath));

	bool Written = false;
	IO::CFileStream File;
	if (File.Open(pOutFilePath, IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
		Written = File.Write(pCode->GetBufferPointer(), pCode->GetBufferSize()) == pCode->GetBufferSize();

	pCode->Release();

	return Written ? SUCCESS : ERR_IO_WRITE;
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

	// Techniques
	//???Samplers?
	// RenderStates (hierarchy) -> save with scheme, strings to enum codes
	// VS - get input blob, check if exists, else save with signature
	// All shaders - register mappings

	return SUCCESS;
}
//---------------------------------------------------------------------
