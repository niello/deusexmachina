#pragma once
#include <map>
#include <istream>
#include <ostream>

// Data and functions for shader metadata manipulation in both SM3.0 and USM

//#define DEM_DLL_EXPORT	__declspec(dllexport)

// Don't change existing values, they are saved to file
enum EShaderModel
{
	ShaderModel_30	= 0,
	ShaderModel_USM	= 1
};

// Don't change existing values, they are saved to file
enum EShaderParamClass
{
	ShaderParam_Const		= 0,
	ShaderParam_Resource	= 1,
	ShaderParam_Sampler		= 2,

	ShaderParam_COUNT,

	ShaderParam_None	// Buffers, structures and other non-param metadata objects
};

enum EShaderConstType
{
	ShaderConst_Bool	= 0,
	ShaderConst_Int,
	ShaderConst_Float,

	ShaderConst_Struct,

	ShaderConst_Invalid
};

enum EShaderConstFlags
{
	ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

// Hardware capability level relative to D3D features.
// This is a hardware attribute, it doesn't depend on API used.
// Never change values, they are used in a file format.
enum EGPUFeatureLevel
{
	GPU_Level_D3D9_1	= 0x9100,
	GPU_Level_D3D9_2	= 0x9200,
	GPU_Level_D3D9_3	= 0x9300,   // OpenGL 3.2 (?)
	GPU_Level_D3D10_0	= 0xa000,   // OpenGL 3.3
	GPU_Level_D3D10_1	= 0xa100,
	GPU_Level_D3D11_0	= 0xb000,   // OpenGL 4.0 (compute shaders from 4.3?)
	GPU_Level_D3D11_1	= 0xb100,
	GPU_Level_D3D12_0	= 0xc000,   // Vulkan
	GPU_Level_D3D12_1	= 0xc100
};

class CMetadataObject
{
public:

	virtual const char*			GetName() const = 0;
	virtual EShaderModel		GetShaderModel() const = 0;
	virtual EShaderParamClass	GetClass() const = 0;
	virtual bool				IsEqual(const CMetadataObject& Other) const = 0;

	bool operator ==(const CMetadataObject& Other) const { return IsEqual(Other); }
	bool operator !=(const CMetadataObject& Other) const { return !IsEqual(Other); }
};

class CShaderMetadata
{
public:

	virtual bool Load(std::istream& Stream) = 0;
	virtual bool Save(std::ostream& Stream) const = 0;
};
