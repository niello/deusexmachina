#pragma once
#ifndef __DEM_TOOLS_DEM_SHADER_COMPILER_DLL_H__
#define __DEM_TOOLS_DEM_SHADER_COMPILER_DLL_H__

#include <DEMShaderCompiler/ShaderCompiler.h>
#include <DEMShaderCompiler/ShaderReflection.h>

bool InitDEMShaderCompilerDLL(const char* pDLLPath, const char* pDBPath);
bool TermDEMShaderCompilerDLL();
bool DLLCompileShader(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
				   const char* pDefines, bool Debug, U32& ObjectFileID, U32& InputSignatureFileID);
bool DLLLoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CSM30ShaderMeta& OutD3D9Meta, CD3D11ShaderMeta& OutD3D11Meta);

#endif
