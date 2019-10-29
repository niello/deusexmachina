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

typedef Ptr<class CUSMConstantParam> PUSMConstantParam;
typedef Ptr<class CUSMConstantBufferParam> PUSMConstantBufferParam;
typedef Ptr<class CUSMResourceParam> PUSMResourceParam;
typedef Ptr<class CUSMSamplerParam> PUSMSamplerParam;
typedef Ptr<class CUSMStructMeta> PUSMStructMeta;
typedef Ptr<class CUSMConstantMeta> PUSMConstantMeta;

class CUSMConstantMeta : public Data::CRefCounted
{
public:

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
	std::vector<PUSMConstantMeta> Members;
};

class CUSMConstantParam : public IShaderConstantParam
{
protected:

	PUSMConstantBufferParam _Buffer;
	PUSMConstantMeta        _Meta;
	U32                     _OffsetInBuffer;

public:

	CUSMConstantParam(PUSMConstantBufferParam Buffer, PUSMConstantMeta Meta, U32 Offset = 0);

	virtual CStrID                GetID() const override { return _Meta->Name; }
	virtual IConstantBufferParam& GetConstantBuffer() const override;
	virtual void                  SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const override;
	//virtual void SetFloats(CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetInts(CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetBools(CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
};

class CUSMConstantBufferParam : public IConstantBufferParam
{
	__DeclareClassNoFactory;

public:

	CStrID         Name;
	EUSMBufferType Type;
	U32            Register;
	U32            Size;           // For structured buffers - StructureByteStride
	U8             ShaderTypeMask;

	virtual CStrID GetID() const override { return Name; }
	virtual bool   Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const override;
	virtual bool   IsBufferCompatible(CConstantBuffer& Value) const override;
};

class CUSMResourceParam : public IResourceParam
{
protected:

	CStrID           _Name;
	EUSMResourceType _Type;
	U32              _RegisterStart;
	U32              _RegisterCount;
	U8               _ShaderTypeMask;

public:

	CUSMResourceParam(CStrID Name, U8 ShaderTypeMask, EUSMResourceType Type, U32 RegisterStart, U32 RegisterCount);

	virtual CStrID GetID() const override { return _Name; }
	virtual bool   Apply(CGPUDriver& GPU, CTexture* pValue) const override;
};

class CUSMSamplerParam : public ISamplerParam
{
protected:

	CStrID _Name;
	U32    _RegisterStart;
	U32    _RegisterCount;
	U8     _ShaderTypeMask;

public:

	CUSMSamplerParam(CStrID Name, U8 ShaderTypeMask, U32 RegisterStart, U32 RegisterCount);

	virtual CStrID GetID() const override { return _Name; }
	virtual bool   Apply(CGPUDriver& GPU, CSampler* pValue) const override;
};

}
