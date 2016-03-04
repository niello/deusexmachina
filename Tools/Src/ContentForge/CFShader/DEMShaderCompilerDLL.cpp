#include "DEMShaderCompilerDLL.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

HMODULE hDLL = NULL;
FDEMShaderCompiler_InitCompiler pInitCompiler = NULL;
FDEMShaderCompiler_CompileShader pCompileShader = NULL;
FDEMShaderCompiler_LoadShaderMetadataByObjectFileID pLoadShaderMetadataByObjectFileID = NULL;
FDEMShaderCompiler_FreeShaderMetadata pFreeShaderMetadata = NULL;

bool InitDEMShaderCompilerDLL(const char* pDLLPath, const char* pDBFilePath, const char* pOutputDirectory)
{
	if (!hDLL)
	{
		hDLL = ::LoadLibrary(pDLLPath);
		if (!hDLL) FAIL;
	}

	pInitCompiler = (FDEMShaderCompiler_InitCompiler)GetProcAddress(hDLL, "InitCompiler");
	if (!pInitCompiler) FAIL;

	return pInitCompiler(pDBFilePath, pOutputDirectory);
}
//---------------------------------------------------------------------

bool TermDEMShaderCompilerDLL()
{
	// DB is closed inside a DLL
	return !hDLL || ::FreeLibrary(hDLL) == TRUE;
}
//---------------------------------------------------------------------

bool DLLCompileShader(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
				   const char* pDefines, bool Debug, U32& ObjectFileID, U32& InputSignatureFileID)
{
	if (!pCompileShader)
	{
		if (!hDLL) FAIL;
		pCompileShader = (FDEMShaderCompiler_CompileShader)GetProcAddress(hDLL, "CompileShader");
		if (!pCompileShader) FAIL;
	}

	return pCompileShader(pSrcPath, ShaderType, Target, pEntryPoint, pDefines, Debug, ObjectFileID, InputSignatureFileID);
}
//---------------------------------------------------------------------

bool DLLLoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CSM30ShaderMeta& OutD3D9Meta, CD3D11ShaderMeta& OutD3D11Meta)
{
	if (!pLoadShaderMetadataByObjectFileID)
	{
		if (!hDLL) FAIL;
		pLoadShaderMetadataByObjectFileID = (FDEMShaderCompiler_LoadShaderMetadataByObjectFileID)GetProcAddress(hDLL, "LoadShaderMetadataByObjectFileID");
		if (!pLoadShaderMetadataByObjectFileID) FAIL;
	}

	if (!pFreeShaderMetadata)
	{
		if (!hDLL) FAIL;
		pFreeShaderMetadata = (FDEMShaderCompiler_FreeShaderMetadata)GetProcAddress(hDLL, "FreeShaderMetadata");
		if (!pFreeShaderMetadata) FAIL;
	}

	CSM30ShaderMeta* pDLLAllocD3D9Meta;
	CD3D11ShaderMeta* pDLLAllocD3D11Meta;
	if (!pLoadShaderMetadataByObjectFileID(ID, OutTarget, pDLLAllocD3D9Meta, pDLLAllocD3D11Meta)) FAIL;
	if (pDLLAllocD3D9Meta) OutD3D9Meta = *pDLLAllocD3D9Meta;
	if (pDLLAllocD3D11Meta) OutD3D11Meta = *pDLLAllocD3D11Meta;
	pFreeShaderMetadata(pDLLAllocD3D9Meta, pDLLAllocD3D11Meta);

	OK;
}
//---------------------------------------------------------------------
