#pragma once
#include <map>
#include <fstream>

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

	virtual bool				Load(std::ifstream& File) = 0;
	virtual bool				Save(std::ofstream& File) const = 0;

	virtual EShaderModel		GetShaderModel() const = 0;
	virtual uint32_t			GetMinFeatureLevel() const = 0;
	virtual void				SetMinFeatureLevel(uint32_t NewLevel) = 0;
	virtual uint64_t			GetRequiresFlags() const = 0;
	virtual void				SetRequiresFlags(uint64_t NewFlags) = 0;

	virtual size_t				GetParamCount(EShaderParamClass Class) const = 0;
	virtual CMetadataObject*	GetParamObject(EShaderParamClass Class, size_t Index) = 0;
	virtual size_t				AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject) = 0;
	virtual bool				FindParamObjectByName(EShaderParamClass Class, const char* pName, size_t& OutIndex) const = 0;

	virtual size_t				AddOrMergeBuffer(const CMetadataObject* pMetaBuffer) = 0;
	virtual CMetadataObject*	GetContainingConstantBuffer(const CMetadataObject* pMetaObject) = 0;
	virtual bool				SetContainingConstantBuffer(size_t ConstIdx, size_t BufferIdx) = 0;

	virtual uint32_t			AddStructure(const CShaderMetadata& SourceMeta, uint64_t StructKey, std::map<uint64_t, uint32_t>& StructIndexMapping) = 0;
	virtual uint32_t			GetStructureIndex(size_t ConstIdx) const = 0;
	virtual bool				SetStructureIndex(size_t ConstIdx, uint32_t StructIdx) = 0;
};

template<class T> void ReadFile(std::ifstream& File, T& Out)
{
	File.read(reinterpret_cast<char*>(&Out), sizeof(T));
}
//---------------------------------------------------------------------

template<class T> void WriteFile(std::ofstream& File, const T& Data)
{
	File.write(reinterpret_cast<const char*>(&Data), sizeof(T));
}
//---------------------------------------------------------------------
