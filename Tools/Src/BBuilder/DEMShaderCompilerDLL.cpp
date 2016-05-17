#include "DEMShaderCompilerDLL.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

HMODULE hDLL = NULL;
FDEMShaderCompiler_InitCompiler pInitCompiler = NULL;
FDEMShaderCompiler_GetLastOperationMessages pGetLastOperationMessages = NULL;
FDEMShaderCompiler_CompileShader pCompileShader = NULL;
FDEMShaderCompiler_LoadShaderMetadataByObjectFileID pLoadShaderMetadataByObjectFileID = NULL;
FDEMShaderCompiler_FreeShaderMetadata pFreeShaderMetadata = NULL;
FDEMShaderCompiler_PackShaders pPackShaders = NULL;

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

// DB is closed inside a DLL
bool TermDEMShaderCompilerDLL()
{
	bool Result = true;
	if (hDLL)
	{
		if (::FreeLibrary(hDLL) != TRUE) Result = false;
		hDLL = 0;
	}

	pInitCompiler = NULL;
	pGetLastOperationMessages = NULL;
	pCompileShader = NULL;
	pLoadShaderMetadataByObjectFileID = NULL;
	pFreeShaderMetadata = NULL;
	pPackShaders = NULL;

	return Result;
}
//---------------------------------------------------------------------

const char*	DLLGetLastOperationMessages()
{
	if (!pGetLastOperationMessages)
	{
		if (!hDLL) return NULL;
		pGetLastOperationMessages = (FDEMShaderCompiler_GetLastOperationMessages)GetProcAddress(hDLL, "GetLastOperationMessages");
		if (!pGetLastOperationMessages) return NULL;
	}

	return pGetLastOperationMessages();
}
//---------------------------------------------------------------------

int DLLCompileShader(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
				   const char* pDefines, bool Debug, U32& ObjectFileID, U32& InputSignatureFileID)
{
	if (!pCompileShader)
	{
		if (!hDLL) return DEM_SHADER_COMPILER_ERROR;
		pCompileShader = (FDEMShaderCompiler_CompileShader)GetProcAddress(hDLL, "CompileShader");
		if (!pCompileShader) return DEM_SHADER_COMPILER_ERROR;
	}

	return pCompileShader(pSrcPath, ShaderType, Target, pEntryPoint, pDefines, Debug, ObjectFileID, InputSignatureFileID);
}
//---------------------------------------------------------------------

bool DLLLoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CSM30ShaderMeta*& pOutD3D9Meta, CD3D11ShaderMeta*& pOutD3D11Meta)
{
	if (!pLoadShaderMetadataByObjectFileID)
	{
		if (!hDLL) FAIL;
		pLoadShaderMetadataByObjectFileID = (FDEMShaderCompiler_LoadShaderMetadataByObjectFileID)GetProcAddress(hDLL, "LoadShaderMetadataByObjectFileID");
		if (!pLoadShaderMetadataByObjectFileID) FAIL;
	}

	return pLoadShaderMetadataByObjectFileID(ID, OutTarget, pOutD3D9Meta, pOutD3D11Meta);
}
//---------------------------------------------------------------------

bool DLLFreeShaderMetadata(CSM30ShaderMeta* pDLLAllocD3D9Meta, CD3D11ShaderMeta* pDLLAllocD3D11Meta)
{
	if (!pFreeShaderMetadata)
	{
		if (!hDLL) FAIL;
		pFreeShaderMetadata = (FDEMShaderCompiler_FreeShaderMetadata)GetProcAddress(hDLL, "FreeShaderMetadata");
		if (!pFreeShaderMetadata) FAIL;
	}

	pFreeShaderMetadata(pDLLAllocD3D9Meta, pDLLAllocD3D11Meta);

	OK;
}
//---------------------------------------------------------------------

unsigned int DLLPackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath)
{
	if (!pPackShaders)
	{
		if (!hDLL) return NULL;
		pPackShaders = (FDEMShaderCompiler_PackShaders)GetProcAddress(hDLL, "PackShaders");
		if (!pPackShaders) return NULL;
	}

	return pPackShaders(pCommaSeparatedShaderIDs, pLibraryFilePath);
}
//---------------------------------------------------------------------
