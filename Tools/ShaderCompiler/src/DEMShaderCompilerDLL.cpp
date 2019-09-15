#include "DEMShaderCompilerDLL.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HMODULE hDLL = nullptr;
static FDEMShaderCompiler_InitCompiler pInitCompiler = nullptr;
static FDEMShaderCompiler_GetLastOperationMessages pGetLastOperationMessages = nullptr;
static FDEMShaderCompiler_CompileShader pCompileShader = nullptr;
static FDEMShaderCompiler_CreateShaderMetadata pCreateShaderMetadata = nullptr;
static FDEMShaderCompiler_LoadShaderMetadataByObjectFileID pLoadShaderMetadataByObjectFileID = nullptr;
static FDEMShaderCompiler_FreeShaderMetadata pFreeShaderMetadata = nullptr;
//static FDEMShaderCompiler_SaveShaderMetadata pSaveShaderMetadata = nullptr;
static FDEMShaderCompiler_PackShaders pPackShaders = nullptr;

bool InitDEMShaderCompilerDLL(const char* pDLLPath, const char* pDBFilePath, const char* pOutputDirectory)
{
	if (!hDLL)
	{
		hDLL = ::LoadLibrary(pDLLPath);
		if (!hDLL) return false;
	}

	pInitCompiler = (FDEMShaderCompiler_InitCompiler)GetProcAddress(hDLL, "InitCompiler");
	if (!pInitCompiler) return false;

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

	pInitCompiler = nullptr;
	pGetLastOperationMessages = nullptr;
	pCompileShader = nullptr;
	pLoadShaderMetadataByObjectFileID = nullptr;
	pFreeShaderMetadata = nullptr;
	pPackShaders = nullptr;

	return Result;
}
//---------------------------------------------------------------------

const char*	DLLGetLastOperationMessages()
{
	if (!pGetLastOperationMessages)
	{
		if (!hDLL) return nullptr;
		pGetLastOperationMessages = (FDEMShaderCompiler_GetLastOperationMessages)GetProcAddress(hDLL, "GetLastOperationMessages");
		if (!pGetLastOperationMessages) return nullptr;
	}

	return pGetLastOperationMessages();
}
//---------------------------------------------------------------------

int DLLCompileShader(const char* pSrcPath, EShaderType ShaderType, uint32_t Target, const char* pEntryPoint,
				   const char* pDefines, bool Debug, bool OnlyMetadata, uint32_t& ObjectFileID, uint32_t& InputSignatureFileID)
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

bool DLLLoadShaderMetadataByObjectFileID(uint32_t ID, uint32_t& OutTarget, CShaderMetadata*& pOutMeta)
{
	if (!pLoadShaderMetadataByObjectFileID)
	{
		if (!hDLL) return false;
		pLoadShaderMetadataByObjectFileID = (FDEMShaderCompiler_LoadShaderMetadataByObjectFileID)GetProcAddress(hDLL, "LoadShaderMetadataByObjectFileID");
		if (!pLoadShaderMetadataByObjectFileID) return false;
	}

	return pLoadShaderMetadataByObjectFileID(ID, OutTarget, pOutMeta);
}
//---------------------------------------------------------------------

bool DLLFreeShaderMetadata(CShaderMetadata* pDLLAllocMeta)
{
	if (!pFreeShaderMetadata)
	{
		if (!hDLL) return false;
		pFreeShaderMetadata = (FDEMShaderCompiler_FreeShaderMetadata)GetProcAddress(hDLL, "FreeShaderMetadata");
		if (!pFreeShaderMetadata) return false;
	}

	pFreeShaderMetadata(pDLLAllocMeta);

	return true;
}
//---------------------------------------------------------------------

unsigned int DLLPackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath)
{
	if (!pPackShaders)
	{
		if (!hDLL) return 0;
		pPackShaders = (FDEMShaderCompiler_PackShaders)GetProcAddress(hDLL, "PackShaders");
		if (!pPackShaders) return 0;
	}

	return pPackShaders(pCommaSeparatedShaderIDs, pLibraryFilePath);
}
//---------------------------------------------------------------------
