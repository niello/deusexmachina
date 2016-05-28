#pragma once
#ifndef __DEM_TOOLS_SHADER_COMPILER_H__
#define __DEM_TOOLS_SHADER_COMPILER_H__

#include <stdint.h> // for uint32_t only

// Main public API header

typedef uint32_t U32;	// Unsigned 32-bit integer
struct CSM30ShaderMeta;
struct CD3D11ShaderMeta;

#define DEM_DLL_EXPORT	__declspec(dllexport)
#define DEM_DLLCALL		__cdecl

// Return codes
enum EReturnCode
{
	DEM_SHADER_COMPILER_SUCCESS				= 0,

	DEM_SHADER_COMPILER_ERROR				= 100,
	DEM_SHADER_COMPILER_INVALID_ARGS,
	DEM_SHADER_COMPILER_COMPILE_ERROR,
	DEM_SHADER_COMPILER_REFLECTION_ERROR,
	DEM_SHADER_COMPILER_IO_READ_ERROR,
	DEM_SHADER_COMPILER_IO_WRITE_ERROR,
	DEM_SHADER_COMPILER_DB_ERROR

	//DEM_SHADER_REFLECTION_DUPLICATE_PARAMETER
};

enum EShaderType
{
	ShaderType_Vertex = 0,	// Don't change order and starting index
	ShaderType_Pixel,
	ShaderType_Geometry,
	ShaderType_Hull,
	ShaderType_Domain,

	ShaderType_COUNT
};

#ifdef __cplusplus
extern "C"
{
#endif

// For static loading
DEM_DLL_EXPORT bool DEM_DLLCALL			InitCompiler(const char* pDBFileName, const char* pOutputDirectory);
DEM_DLL_EXPORT const char* DEM_DLLCALL	GetLastOperationMessages();
DEM_DLL_EXPORT int DEM_DLLCALL			CompileShader(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
													  const char* pDefines, bool Debug, bool OnlyMetadata, U32& ObjectFileID, U32& InputSignatureFileID);
DEM_DLL_EXPORT bool DEM_DLLCALL			LoadShaderMetadataByObjectFileID(U32 ID, U32& OutTarget, CSM30ShaderMeta*& pOutD3D9Meta, CD3D11ShaderMeta*& pOutD3D11Meta);
DEM_DLL_EXPORT void DEM_DLLCALL			FreeShaderMetadata(CSM30ShaderMeta* pD3D9Meta, CD3D11ShaderMeta* pD3D11Meta);
DEM_DLL_EXPORT unsigned int DEM_DLLCALL	PackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath);

#ifdef __cplusplus
}
#endif

// For dynamic loading
typedef bool (DEM_DLLCALL *FDEMShaderCompiler_InitCompiler)(const char* pDBFileName, const char* pOutputDirectory);
typedef const char* (DEM_DLLCALL *FDEMShaderCompiler_GetLastOperationMessages)();
typedef int (DEM_DLLCALL *FDEMShaderCompiler_CompileShader)(const char* pSrcPath, EShaderType ShaderType, U32 Target, const char* pEntryPoint,
															const char* pDefines, bool Debug, bool OnlyMetadata, U32& ObjectFileID, U32& InputSignatureFileID);
typedef bool (DEM_DLLCALL *FDEMShaderCompiler_LoadShaderMetadataByObjectFileID)(U32 ID, U32& OutTarget, CSM30ShaderMeta*& pOutD3D9Meta, CD3D11ShaderMeta*& pOutD3D11Meta);
typedef void (DEM_DLLCALL *FDEMShaderCompiler_FreeShaderMetadata)(CSM30ShaderMeta* pD3D9Meta, CD3D11ShaderMeta* pD3D11Meta);
typedef unsigned int (DEM_DLLCALL *FDEMShaderCompiler_PackShaders)(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath);

#endif
