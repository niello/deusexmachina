#pragma once
#include <Data/RefCounted.h>

// Shader parameter table stores metadata necessary to set shader parameter values

namespace Render
{
typedef Ptr<class IShaderConstantParam> PShaderConstantParam;
typedef Ptr<class IShaderConstantBufferParam> PShaderConstantBufferParam;
typedef Ptr<class IShaderResourceParam> PShaderResourceParam;
typedef Ptr<class IShaderSamplerParam> PShaderSamplerParam;
typedef Ptr<class CShaderParamTable> PShaderParamTable;

class IShaderConstantParam : public Data::CRefCounted
{
public:

	virtual void SetRawValue(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetFloats(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetInts(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetBools(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
};

class IShaderConstantBufferParam : public Data::CRefCounted
{
public:

	virtual void Apply(const CGPUDriver& GPU, CConstantBuffer* pValue) const = 0;
};

class IShaderResourceParam : public Data::CRefCounted
{
public:

	virtual void Apply(const CGPUDriver& GPU, CTexture* pValue) const = 0;
};

class IShaderSamplerParam : public Data::CRefCounted
{
public:

	virtual void Apply(const CGPUDriver& GPU, CSampler* pValue) const = 0;
};

class CShaderParamTable : public Data::CRefCounted
{
public:

	// Vectors are sorted and never change in runtime
	std::vector<PShaderConstantParam> Constants;
	std::vector<PShaderConstantBufferParam> ConstantBuffers;
	std::vector<PShaderResourceParam> Resources;
	std::vector<PShaderSamplerParam> Samplers;

	size_t GetConstantIndex(CStrID ID) const;
	size_t GetResourceIndex(CStrID ID) const;
	size_t GetSamplerIndex(CStrID ID) const;

	const IShaderConstantParam* GetConstant(size_t Index) const;
	const IShaderResourceParam* GetResource(size_t Index) const;
	const IShaderSamplerParam*  GetSampler(size_t Index) const;

	const IShaderConstantParam* GetConstant(CStrID ID) const { return GetConstant(GetConstantIndex(ID)); }
	const IShaderResourceParam* GetResource(CStrID ID) const { return GetResource(GetResourceIndex(ID)); }
	const IShaderSamplerParam*  GetSampler(CStrID ID) const { return GetSampler(GetSamplerIndex(ID)); }
};

}
