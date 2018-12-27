#include "DEMShaderCompilerDLL.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HMODULE hDLL = NULL;
static FDEMShaderCompiler_InitCompiler pInitCompiler = NULL;
static FDEMShaderCompiler_GetLastOperationMessages pGetLastOperationMessages = NULL;
static FDEMShaderCompiler_CompileShader pCompileShader = NULL;
static FDEMShaderCompiler_CreateShaderMetadata pCreateShaderMetadata = NULL;
static FDEMShaderCompiler_LoadShaderMetadataByObjectFileID pLoadShaderMetadataByObjectFileID = NULL;
static FDEMShaderCompiler_FreeShaderMetadata pFreeShaderMetadata = NULL;
//static FDEMShaderCompiler_SaveShaderMetadata pSaveShaderMetadata = NULL;
static FDEMShaderCompiler_PackShaders pPackShaders = NULL;

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
				   const char* pDefines, bool Debug, bool OnlyMetadata, U32& ObjectFileID, U32& InputSignatureFileID)
{
	if (!pCompileShader)
	{
		if (!hDLL) return DEM_SHADER_COMPILER_ERROR;
		pCompileShader = (FDEMShaderCompiler_CompileShader)GetProcAddress(hDLL, "CompileShader");
		if (!pCompileShader) return DEM_SHADER_COMPILER_ERROR;
	}

	return pCompileShader(pSrcPath, ShaderType, Target, pEntryPoint, pDefines, Debug, OnlyMetadata, ObjectFileID, InputSignatureFileID);
}
//---------------------------------------------------------------------

void DLLCreateShaderMetadata(EShaderModel ShaderModel, CShaderMetadata*& pOutMeta)
{
	if (!pCreateShaderMetadata)
	{
		if (!hDLL) return;
		pCreateShaderMetadata = (FDEMShaderCompiler_CreateShaderMetadata)GetProcAddress(hDLL, "CreateShaderMetadata");
		if (!pCreateShaderMetadata) return;
	}

	pCreateShaderMetadata(ShaderModel, pOutMeta);
}
//---------------------------------------------------------------------

bool DLLLoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CShaderMetadata*& pOutMeta)
{
	if (!pLoadShaderMetadataByObjectFileID)
	{
		if (!hDLL) FAIL;
		pLoadShaderMetadataByObjectFileID = (FDEMShaderCompiler_LoadShaderMetadataByObjectFileID)GetProcAddress(hDLL, "LoadShaderMetadataByObjectFileID");
		if (!pLoadShaderMetadataByObjectFileID) FAIL;
	}

	return pLoadShaderMetadataByObjectFileID(ID, OutTarget, pOutMeta);
}
//---------------------------------------------------------------------

bool DLLFreeShaderMetadata(CShaderMetadata* pDLLAllocMeta)
{
	if (!pFreeShaderMetadata)
	{
		if (!hDLL) FAIL;
		pFreeShaderMetadata = (FDEMShaderCompiler_FreeShaderMetadata)GetProcAddress(hDLL, "FreeShaderMetadata");
		if (!pFreeShaderMetadata) FAIL;
	}

	pFreeShaderMetadata(pDLLAllocMeta);

	OK;
}
//---------------------------------------------------------------------

/*
bool DLLSaveShaderMetadata(IO::CBinaryWriter& W, const CShaderMetadata& Meta)
{
	if (!pSaveShaderMetadata)
	{
		if (!hDLL) FAIL;
		pSaveShaderMetadata = (FDEMShaderCompiler_SaveShaderMetadata)GetProcAddress(hDLL, "SaveUSMShaderMetadata");
		if (!pSaveShaderMetadata) FAIL;
	}

	return pSaveShaderMetadata(W, Meta);
}
//---------------------------------------------------------------------
*/

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
