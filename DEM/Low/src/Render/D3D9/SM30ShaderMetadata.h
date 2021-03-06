#pragma once
#include <Render/ShaderParamTable.h>
#include <Data/StringID.h>

// Shader Model 3.0 (for Direct3D 9.0c) shader metadata

namespace Render
{

// Don't change values
enum ESM30RegisterSet
{
	Reg_Bool    = 0,
	Reg_Int4    = 1,
	Reg_Float4  = 2,

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

typedef Ptr<class CSM30ConstantInfo> PSM30ConstantInfo;
typedef Ptr<class CSM30ConstantBufferParam> PSM30ConstantBufferParam;
typedef Ptr<class CSM30ResourceParam> PSM30ResourceParam;
typedef Ptr<class CSM30SamplerParam> PSM30SamplerParam;

class CSM30ConstantInfo : public CShaderConstantInfo
{
protected:

	virtual PShaderConstantInfo Clone() const override;

public:

	ESM30RegisterSet RegisterSet;

	virtual void CalculateCachedValues() override;

	virtual void SetRawValue(CConstantBuffer& CB, U32 Offset, const void* pValue, UPTR Size) const override;
	virtual void SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const override;
	virtual void SetInts(CConstantBuffer& CB, U32 Offset, const I32* pValue, UPTR Count) const override;
	virtual void SetUInts(CConstantBuffer& CB, U32 Offset, const U32* pValue, UPTR Count) const override;
	virtual void SetBools(CConstantBuffer& CB, U32 Offset, const bool* pValue, UPTR Count) const override;
};

class CSM30ConstantBufferParam : public IConstantBufferParam
{
	RTTI_CLASS_DECL(CSM30ConstantBufferParam, IConstantBufferParam);

public:

	// NB: range is a (start, count) pair
	typedef std::vector<std::pair<U32, U32>> CRanges;

	CStrID   Name;
	U32      SlotIndex; // Pseudo-register
	CRanges  Float4;
	CRanges  Int4;
	CRanges  Bool;
	U8       ShaderTypeMask;

	virtual CStrID GetID() const override { return Name; }
	virtual bool   Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const override;
	virtual void   Unapply(CGPUDriver& GPU, CConstantBuffer* pValue) const override;
	virtual bool   IsBufferCompatible(CConstantBuffer& Value) const override;
};

class CSM30ResourceParam : public IResourceParam
{
protected:

	CStrID _Name;
	U32    _Register;
	U8     _ShaderTypeMask;

public:

	CSM30ResourceParam(CStrID Name, U8 ShaderTypeMask, U32 Register);

	virtual CStrID GetID() const override { return _Name; }
	virtual bool   Apply(CGPUDriver& GPU, CTexture* pValue) const override;
	virtual void   Unapply(CGPUDriver& GPU, CTexture* pValue) const override;
};

class CSM30SamplerParam : public ISamplerParam
{
protected:

	CStrID           _Name;
	ESM30SamplerType _Type;
	U32              _RegisterStart;
	U32              _RegisterCount;
	U8               _ShaderTypeMask;

public:

	CSM30SamplerParam(CStrID Name, U8 ShaderTypeMask, ESM30SamplerType Type, U32 RegisterStart, U32 RegisterCount);

	virtual CStrID GetID() const override { return _Name; }
	virtual bool   Apply(CGPUDriver& GPU, CSampler* pValue) const override;
	virtual void   Unapply(CGPUDriver& GPU, CSampler* pValue) const override;
};

}
