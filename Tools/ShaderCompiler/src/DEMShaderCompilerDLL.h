#pragma once
#include <ShaderCompiler.h>
#include <ShaderReflectionSM30.h>
#include <ShaderReflectionUSM.h>

// Convenient wrappers for calling shader compiler DLL functions from an application.
// Include accompanying DEMShaderCompilerDLL.cpp file into a project from where a DLL will be called.

//bool			InitDEMShaderCompilerDLL(const char* pDLLPath, const char* pDBFilePath, const char* pOutputDirectory);
//bool			TermDEMShaderCompilerDLL();
//const char*		DLLGetLastOperationMessages();
//int				DLLCompileShader(const char* pSrcPath, EShaderType ShaderType, uint32_t Target, const char* pEntryPoint,
//								 const char* pDefines, bool Debug, bool OnlyMetadata, uint32_t& ObjectFileID, uint32_t& InputSignatureFileID);
//void			DLLCreateShaderMetadata(EShaderModel ShaderModel, CShaderMetadata*& pOutMeta);
//bool			DLLLoadShaderMetadataByObjectFileID(uint32_t ID, uint32_t& OutTarget, CShaderMetadata*& pOutMeta);
//bool			DLLFreeShaderMetadata(CShaderMetadata* pDLLAllocMeta);
////bool			DLLSaveShaderMetadata(IO::CBinaryWriter& W, const CShaderMetadata& Meta);
//unsigned int	DLLPackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath);
