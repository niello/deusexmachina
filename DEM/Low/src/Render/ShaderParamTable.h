#pragma once
#include <Render/RenderFwd.h>
#include <Core/Object.h>

// Shader parameter table stores metadata necessary to set shader parameter values

namespace Render
{

class IShaderConstantParam : public Data::CRefCounted
{
public:

	virtual CStrID GetID() const = 0;
	virtual void   SetRawValue(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetFloats(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetInts(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	//virtual void SetBools(const CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
};

class IConstantBufferParam : public Core::CObject
{
	__DeclareClassNoFactory;

public:

	virtual CStrID GetID() const = 0;
	virtual bool   Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const = 0;
	virtual bool   IsBufferCompatible(CConstantBuffer& Value) const = 0;
};

class IResourceParam : public Data::CRefCounted
{
public:

	virtual CStrID GetID() const = 0;
	virtual bool   Apply(CGPUDriver& GPU, CTexture* pValue) const = 0;
};

class ISamplerParam : public Data::CRefCounted
{
public:

	virtual CStrID GetID() const = 0;
	virtual bool   Apply(CGPUDriver& GPU, CSampler* pValue) const = 0;
};

class CShaderParamTable : public Data::CRefCounted
{
protected:

	// Vectors are sorted and never change in runtime
	std::vector<PShaderConstantParam> _Constants;
	std::vector<PConstantBufferParam> _ConstantBuffers;
	std::vector<PResourceParam>       _Resources;
	std::vector<PSamplerParam>        _Samplers;

public:

	CShaderParamTable(std::vector<PShaderConstantParam>&& Constants,
		std::vector<PConstantBufferParam>&& ConstantBuffers,
		std::vector<PResourceParam>&& Resources,
		std::vector<PSamplerParam>&& Samplers);

	size_t GetConstantIndex(CStrID ID) const;
	size_t GetConstantBufferIndex(CStrID ID) const;
	size_t GetResourceIndex(CStrID ID) const;
	size_t GetSamplerIndex(CStrID ID) const;

	const IShaderConstantParam* GetConstant(size_t Index) const;
	const IConstantBufferParam* GetConstantBuffer(size_t Index) const;
	const IResourceParam*       GetResource(size_t Index) const;
	const ISamplerParam*        GetSampler(size_t Index) const;

	const IShaderConstantParam* GetConstant(CStrID ID) const { return GetConstant(GetConstantIndex(ID)); }
	const IConstantBufferParam* GetConstantBuffer(CStrID ID) const { return GetConstantBuffer(GetConstantBufferIndex(ID)); }
	const IResourceParam*       GetResource(CStrID ID) const { return GetResource(GetResourceIndex(ID)); }
	const ISamplerParam*        GetSampler(CStrID ID) const { return GetSampler(GetSamplerIndex(ID)); }

	const auto&                 GetConstants() const { return _Constants; }
	const auto&                 GetConstantBuffers() const { return _ConstantBuffers; }
	const auto&                 GetResources() const { return _Resources; }
	const auto&                 GetSamplers() const { return _Samplers; }
};

}
