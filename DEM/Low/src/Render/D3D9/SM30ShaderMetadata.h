#pragma once
#include <Render/ShaderParamTable.h>
#include <Data/StringID.h>

// Shader Model 3.0 (for Direct3D 9.0c) shader metadata

namespace Render
{

// Don't change values
enum ESM30RegisterSet
{
	Reg_Bool			= 0,
	Reg_Int4			= 1,
	Reg_Float4			= 2,

	Reg_Invalid
};

// Don't change values
enum ESM30SamplerType
{
	SM30Sampler_1D		= 0,
	SM30Sampler_2D,
	SM30Sampler_3D,
	SM30Sampler_CUBE
};

// Don't change values
enum ESM30ShaderConstFlags
{
	ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

typedef Ptr<class CSM30BufferMeta> PSM30BufferMeta;
typedef Ptr<class CSM30StructMeta> PSM30StructMeta;

class CSM30BufferMeta : public Data::CRefCounted
{
public:

	// NB: range is a (start, count) pair
	typedef std::vector<std::pair<U32, U32>> CRanges;

	CStrID   Name;
	uint32_t SlotIndex; // Pseudo-register
	CRanges  Float4;
	CRanges  Int4;
	CRanges  Bool;
	HHandle  Handle;
};

struct CSM30ConstMetaBase
{
	CStrID          Name;

	PSM30StructMeta Struct;

	uint32_t        RegisterStart; // For structs - offset from the start of the structure
	uint32_t        ElementRegisterCount;
	uint32_t        ElementCount;
	uint8_t         Columns;
	uint8_t         Rows;
	uint8_t         Flags; // See ESM30ShaderConstFlags

	//ESM30RegisterSet	RegisterSet; //???save for struct members and add mixed-type structure support?
};

class CSM30StructMeta : public Data::CRefCounted
{
public:

	//CStrID Name;
	std::vector<CSM30ConstMetaBase> Members;
};

struct CSM30ConstMeta : public CSM30ConstMetaBase
{
	PSM30BufferMeta  Buffer;
	ESM30RegisterSet RegisterSet;
};

struct CSM30ResourceMeta
{
	CStrID   Name;
	uint32_t Register;
};

struct CSM30SamplerMeta
{
	CStrID           Name;
	ESM30SamplerType Type;
	uint32_t         RegisterStart;
	uint32_t         RegisterCount;
};

class CSM30ShaderConstantParam : public IShaderConstantParam
{
public:

	virtual void SetRawValue(const CConstantBuffer& CB, const void* pValue, UPTR Size) const override;
};

class CSM30ShaderResourceParam : public IShaderResourceParam
{
public:

	virtual void Apply(const CGPUDriver& GPU, CTexture* pValue) const override;
};

class CSM30ShaderSamplerParam : public IShaderSamplerParam
{
public:

	virtual void Apply(const CGPUDriver& GPU, CSampler* pValue) const override;
};

}
