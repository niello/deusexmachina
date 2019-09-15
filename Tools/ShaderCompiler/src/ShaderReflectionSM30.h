#pragma once
#include <ShaderReflection.h>
#include <vector>
#include <set>

// Legacy Shader Model 3.0 (SM3.0) metadata

// Don't change existing values, they are saved to file
enum ESM30SamplerType
{
	SM30Sampler_1D		= 0,
	SM30Sampler_2D,
	SM30Sampler_3D,
	SM30Sampler_CUBE
};

// Don't change existing values, they are saved to file
enum ESM30RegisterSet
{
	RS_Bool		= 0,
	RS_Int4		= 1,
	RS_Float4	= 2
};

class CSM30BufferMeta: public CMetadataObject
{
public:

	std::string			Name;
	uint32_t			SlotIndex;	// Pseudo-register, two buffers at the same slot will conflict

	// NB: it is very important that they are sorted and have unique elements!
	std::set<uint32_t>	UsedFloat4;
	std::set<uint32_t>	UsedInt4;
	std::set<uint32_t>	UsedBool;

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CSM30StructMemberMeta: public CMetadataObject
{
public:

	std::string	Name;
	uint32_t	StructIndex;
	uint32_t	RegisterOffset;
	uint32_t	ElementRegisterCount;
	uint32_t	ElementCount;
	uint8_t		Columns;
	uint8_t		Rows;
	uint8_t		Flags;					// See EShaderConstFlags

	//???store register set and support mixed structs?

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { return false; } // No need
};

class CSM30StructMeta: public CMetadataObject
{
public:

	//std::string						Name;
	std::vector<CSM30StructMemberMeta>	Members;

	virtual const char*			GetName() const { return nullptr; } // Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_None; }
	virtual bool				IsEqual(const CMetadataObject& Other) const { return false; } // No need
};

// Arrays and single-type structures are supported
class CSM30ConstMeta: public CMetadataObject
{
public:

	std::string			Name;
	uint32_t			BufferIndex;
	uint32_t			StructIndex;
	ESM30RegisterSet	RegisterSet;
	uint32_t			RegisterStart;
	uint32_t			ElementRegisterCount;
	uint32_t			ElementCount;
	uint8_t				Columns;
	uint8_t				Rows;
	uint8_t				Flags;					// See EShaderConstFlags

	uint32_t			RegisterCount;			// Cache, not saved

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Const; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

// Arrays aren't supported, one texture to multiple samplers isn't supported yet
//???what about one tex to multiple samplers? store register bit mask or 'UsedRegisters' array?
class CSM30RsrcMeta: public CMetadataObject
{
public:

	std::string	Name;
	uint32_t		Register;

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Resource; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

// Arrays supported with arbitrarily assigned textures
class CSM30SamplerMeta: public CMetadataObject
{
public:

	std::string				Name;
	ESM30SamplerType	Type;
	uint32_t					RegisterStart;
	uint32_t					RegisterCount;

	virtual const char*			GetName() const { return Name.c_str(); }
	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual EShaderParamClass	GetClass() const { return ShaderParam_Sampler; }
	virtual bool				IsEqual(const CMetadataObject& Other) const;
};

class CDEMD3DInclude;

class CSM30ShaderMeta: public CShaderMetadata
{
public:

	std::vector<CSM30BufferMeta>	Buffers;
	std::vector<CSM30StructMeta>	Structs;
	std::vector<CSM30ConstMeta>		Consts;
	std::vector<CSM30RsrcMeta>		Resources;
	std::vector<CSM30SamplerMeta>	Samplers;

	bool						CollectFromBinaryAndSource(const void* pData, size_t Size, const char* pSource, size_t SourceSize, CDEMD3DInclude& IncludeHandler);

	virtual bool				Load(std::ifstream& File) override;
	virtual bool				Save(std::ofstream& File) const override;

	virtual EShaderModel		GetShaderModel() const { return ShaderModel_30; }
	virtual uint32_t			GetMinFeatureLevel() const;
	virtual void				SetMinFeatureLevel(uint32_t NewLevel) { }
	virtual uint64_t			GetRequiresFlags() const { return 0; }
	virtual void				SetRequiresFlags(uint64_t NewFlags) { }

	virtual size_t				GetParamCount(EShaderParamClass Class) const;
	virtual CMetadataObject*	GetParamObject(EShaderParamClass Class, size_t Index);
	virtual size_t				AddParamObject(EShaderParamClass Class, const CMetadataObject* pMetaObject);
	virtual bool				FindParamObjectByName(EShaderParamClass Class, const char* pName, size_t& OutIndex) const;
};
