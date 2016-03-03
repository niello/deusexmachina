#include "DEMShaderCompilerDLL.h"

#include <IO/IOServer.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

HMODULE hDLL = NULL;
FDEMShaderCompiler_OpenShaderDatabase pOpenShaderDatabase = NULL;
FDEMShaderCompiler_CompileShader pCompileShader = NULL;
FDEMShaderCompiler_LoadShaderMetadataByObjectFileID pLoadShaderMetadataByObjectFileID = NULL;

bool InitDEMShaderCompilerDLL(const char* pDLLPath, const char* pDBPath)
{
	if (!hDLL)
	{
		hDLL = ::LoadLibrary(pDLLPath);
		if (!hDLL) FAIL;
	}

	pOpenShaderDatabase = (FDEMShaderCompiler_OpenShaderDatabase)GetProcAddress(hDLL, "OpenShaderDatabase");
	if (!pOpenShaderDatabase) FAIL;

	return pOpenShaderDatabase(pDBPath);
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

	return pLoadShaderMetadataByObjectFileID(ID, OutTarget, OutD3D9Meta, OutD3D11Meta);
}
//---------------------------------------------------------------------
