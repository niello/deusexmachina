#pragma once
#include <Data/RefCounted.h>

// Shader parameter table stores metadata necessary to set shader parameter values

namespace Render
{
typedef std::unique_ptr<class IShaderConstantParam> PShaderConstantParam;
typedef std::unique_ptr<class IShaderResourceParam> PShaderResourceParam;
typedef std::unique_ptr<class IShaderSamplerParam> PShaderSamplerParam;
typedef Ptr<class CShaderParamTable> PShaderParamTable;

//???interfaces or base classes with common data?
//???need refcounting or always use raw pointers in clients?

class IShaderConstantParam
{
public:

	virtual void SetRawValue(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
};

class IShaderResourceParam
{
public:

	virtual void Apply(const CGPUDriver& GPU, CTexture* pValue) const = 0;
};

class IShaderSamplerParam
{
public:

	virtual void Apply(const CGPUDriver& GPU, CSampler* pValue) const = 0;
};

class CShaderParamTable : public Data::CRefCounted
{
protected:

	//!!!sorted vectors for stable indices + fast search by CStrID! index is handle.

	// struct info for constants
	// constants
	// cbs
	// resources
	// samplers

	//???each of params stores its index inside to avoid search? it is like a handle
	//internal dbg validation is simple, pParam == Table.pParams[pParam->Index]

public:

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
