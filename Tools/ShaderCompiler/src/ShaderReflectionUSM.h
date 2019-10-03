#pragma once
#include <ShaderReflection.h>
#include <vector>

// Universal Shader Model (SM4.0 and higher) metadata

struct ID3D11ShaderReflectionType;

// Don't change existing values, they are saved to file
enum EUSMBufferTypeMask
{
	USMBuffer_Texture		= (1 << 30),
	USMBuffer_Structured	= (2 << 30),

	USMBuffer_RegisterMask = ~(USMBuffer_Texture | USMBuffer_Structured)
};

// Don't change existing values, they are saved to file
enum EUSMConstType
{
	USMConst_Bool	= 0,
	USMConst_Int,
	USMConst_Float,

	USMConst_Struct,

	USMConst_Invalid
};

// Don't change existing values, they are saved to file
enum EUSMResourceType
{
	USMRsrc_Texture1D			= 0,
	USMRsrc_Texture1DArray,
	USMRsrc_Texture2D,
	USMRsrc_Texture2DArray,
	USMRsrc_Texture2DMS,
	USMRsrc_Texture2DMSArray,
	USMRsrc_Texture3D,
	USMRsrc_TextureCUBE,
	USMRsrc_TextureCUBEArray,

	USMRsrc_Unknown
};

class CUSMBufferMeta: public CMetadataObject
{
public:

	std::string	Name;
	uint32_t	Register;
	uint32_t	Size;		// For structured buffers - StructureByteStride

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMStructMemberMeta: public CMetadataObject
{
public:

	std::string		Name;
	uint32_t		StructIndex;
	EUSMConstType	Type;
	uint32_t		Offset;
	uint32_t		ElementSize;
	uint32_t		ElementCount;
	uint8_t			Columns;
	uint8_t			Rows;
	uint8_t			Flags;			// See EShaderConstFlags

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { return false; } // No need
};

class CUSMStructMeta: public CMetadataObject
{
public:

	//std::string						Name;
	std::vector<CUSMStructMemberMeta>	Members;

	virtual const char*			GetName() const { return nullptr; } // Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { return false; } // No need
};

// Arrays and mixed-type structs supported
class CUSMConstMeta: public CMetadataObject
{
public:

	std::string		Name;
	uint32_t		BufferIndex;
	uint32_t		StructIndex;
	EUSMConstType	Type;
	uint32_t		Offset;
	uint32_t		ElementSize;
	uint32_t		ElementCount;
	uint8_t			Columns;
	uint8_t			Rows;
	uint8_t			Flags;			// See EShaderConstFlags

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Const; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMRsrcMeta: public CMetadataObject
{
public:

	std::string			Name;
	EUSMResourceType	Type;
	uint32_t			RegisterStart;
	uint32_t			RegisterCount;

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Resource; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMSamplerMeta: public CMetadataObject
{
public:

	std::string	Name;
	uint32_t	RegisterStart;
	uint32_t	RegisterCount;

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_USM; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Sampler; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CUSMShaderMeta: public CShaderMetadata
{
private:

	bool ProcessStructure(ID3D11ShaderReflectionType* pType, uint32_t StructSize, std::map<ID3D11ShaderReflectionType*, size_t>& StructCache);

public:

	std::vector<CUSMBufferMeta>		Buffers;
	std::vector<CUSMStructMeta>		Structs;
	std::vector<CUSMConstMeta>		Consts;
	std::vector<CUSMRsrcMeta>		Resources;
	std::vector<CUSMSamplerMeta>	Samplers;

	uint32_t MinFeatureLevel = 0;
	uint64_t RequiresFlags = 0;

	bool CollectFromBinary(const void* pData, size_t Size);

	virtual bool Load(std::istream& Stream) override;
	virtual bool Save(std::ostream& Stream) const override;
};
