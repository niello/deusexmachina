#pragma once
#include <Render/ShaderParamTable.h>
#include <Data/StringID.h>

// Universal Shader Model (4.0 and higher, for Direct3D 10 and higher) shader metadata

namespace Render
{

// Don't change values
enum EUSMBufferType
{
	USMBuffer_Constant   = 0,
	USMBuffer_Texture    = 1,
	USMBuffer_Structured = 2
};

// Don't change values
enum EUSMConstType
{
	USMConst_Bool	= 0,
	USMConst_Int,
	USMConst_Float,

	USMConst_Struct,

	USMConst_Invalid
};

// Don't change values
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

// Don't change values
enum EUSMShaderConstFlags
{
	ShaderConst_ColumnMajor	= 0x01 // Only for matrix types
};

typedef Ptr<class CUSMShaderConstantParam> PUSMShaderConstantParam;
typedef Ptr<class CUSMShaderConstantBufferParam> PUSMShaderConstantBufferParam;
typedef Ptr<class CUSMShaderResourceParam> PUSMShaderResourceParam;
typedef Ptr<class CUSMShaderSamplerParam> PUSMShaderSamplerParam;
typedef Ptr<class CUSMStructMeta> PUSMStructMeta;
typedef Ptr<class CUSMConstMeta> PUSMConstMeta;

struct CUSMConstMeta
{
	CStrID         Name;

	PUSMStructMeta Struct;

	EUSMConstType  Type;
	U32            Offset; // From the start of CB, for struct members - from the start of the structure
	U32            ElementSize;
	U32            ElementCount;
	U8             Columns;
	U8             Rows;
	U8             Flags; // See EUSMShaderConstFlags
};

class CUSMStructMeta : public Data::CRefCounted
{
public:

	//CStrID Name;
	std::vector<PUSMConstMeta> Members;
};

class CUSMShaderConstantParam : public IShaderConstantParam
{
public:

	PUSMShaderConstantBufferParam Buffer;
	PUSMConstMeta                 Meta;
	U32                           OffsetInBuffer;

	virtual void SetRawValue(const CConstantBuffer& CB, const void* pValue, UPTR Size) const override;
	//virtual void SetFloats(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetInts(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetBools(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
};

class CUSMShaderConstantBufferParam : public IShaderConstantBufferParam
{
public:

	CStrID         Name;
	EUSMBufferType Type;
	U32            Register;
	U32            Size;     // For structured buffers - StructureByteStride

	virtual void Apply(const CGPUDriver& GPU, CConstantBuffer* pValue) const override;
};

class CUSMShaderResourceParam : public IShaderResourceParam
{
protected:

	CStrID           _Name;
	EUSMResourceType _Type;
	U32              _RegisterStart;
	U32              _RegisterCount;
	U8               _ShaderTypeMask;

public:

	CUSMShaderResourceParam(CStrID Name, U8 ShaderTypeMask, EUSMResourceType Type, U32 RegisterStart, U32 RegisterCount);

	virtual void Apply(const CGPUDriver& GPU, CTexture* pValue) const override;
};

class CUSMShaderSamplerParam : public IShaderSamplerParam
{
protected:

	CStrID _Name;
	U32    _RegisterStart;
	U32    _RegisterCount;
	U8     _ShaderTypeMask;

public:

	CUSMShaderSamplerParam(CStrID Name, U8 ShaderTypeMask, U32 RegisterStart, U32 RegisterCount);

	virtual void Apply(const CGPUDriver& GPU, CSampler* pValue) const override;
};

}
