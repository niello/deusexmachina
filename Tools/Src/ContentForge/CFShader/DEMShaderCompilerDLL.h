#pragma once
#ifndef __DEM_TOOLS_DEM_SHADER_COMPILER_DLL_H__
#define __DEM_TOOLS_DEM_SHADER_COMPILER_DLL_H__

#include <DEMShaderCompiler/ShaderCompiler.h>
#include <DEMShaderCompiler/ShaderReflection.h>

bool		InitDEMShaderCompilerDLL(const char* pDLLPath, const char* pDBFilePath, const char* pOutputDirectory);
bool		TermDEMShaderCompilerDLL();
const char*	DLLGetLastOperationMessages();
int			DLLCompileShader(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
							 const char* pDefines, bool Debug, U32& ObjectFileID, U32& InputSignatureFileID);
bool		DLLLoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CSM30ShaderMeta*& pOutD3D9Meta, CD3D11ShaderMeta*& pOutD3D11Meta);
bool		DLLFreeShaderMetadata(CSM30ShaderMeta* pDLLAllocD3D9Meta, CD3D11ShaderMeta* pDLLAllocD3D11Meta);

#endif
