#pragma once
#include <LogDelegate.h>
#include <string>

// Main public API header

#ifdef _WINDLL
#define DEM_DLL_API	__declspec(dllexport)
#else
#define DEM_DLL_API	__declspec(dllimport)
#endif
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

namespace DEMShaderCompiler
{

// For static loading
DEM_DLL_API bool DEM_DLLCALL Init(const char* pDBFileName);
DEM_DLL_API int DEM_DLLCALL CompileShader(const char* pBasePath, const char* pSrcPath, const char* pDestPath, const char* pInputSigDir,
	EShaderType ShaderType, uint32_t Target, const char* pEntryPoint, const char* pDefines, bool Debug, bool Recompile,
	const char* pSrcData = nullptr, size_t SrcDataSize = 0, ILogDelegate* pLog = nullptr);
DEM_DLL_API uint32_t DEM_DLLCALL PackShaders(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath);

}

//// For dynamic loading
////???need extern "C"?
//typedef bool (DEM_DLLCALL *FDEMShaderCompiler_InitCompiler)(const char* pDBFileName, const char* pOutputDirectory);
//typedef int (DEM_DLLCALL *FDEMShaderCompiler_CompileShader)(const char* pSrcPath, EShaderType ShaderType, uint32_t Target, const char* pEntryPoint,
//															const char* pDefines, bool Debug, bool OnlyMetadata, uint32_t& ObjectFileID, uint32_t& InputSignatureFileID);
//typedef uint32_t (DEM_DLLCALL *FDEMShaderCompiler_PackShaders)(const char* pCommaSeparatedShaderIDs, const char* pLibraryFilePath);
