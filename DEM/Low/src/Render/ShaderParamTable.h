#pragma once
#include <Render/RenderFwd.h>
#include <Core/Object.h>

// Shader parameter table stores metadata necessary to set shader parameter values

#ifdef _DEBUG
#define DEM_SHADER_META_DYNAMIC_TYPE_VALIDATION (1)
#else
#define DEM_SHADER_META_DYNAMIC_TYPE_VALIDATION (0)
#endif

namespace Render
{

class IShaderConstantInfo : public Core::CObject
{
public:

	virtual CStrID GetID() const = 0;
	virtual U32    GetLocalOffset() const = 0;
	virtual U32    GetElementCount() const = 0;
	virtual U32    GetElementStride() const = 0;
	virtual U32    GetComponentStride() const = 0;
	virtual bool   HasElementPadding() const = 0;
	virtual bool   NeedConversionFrom(/*type*/) const = 0;

	virtual void   SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const = 0;

	virtual PShaderConstantInfo GetMemberInfo(const char* pName) const = 0;
	virtual PShaderConstantInfo GetElementInfo() const = 0;
	virtual PShaderConstantInfo GetComponentInfo() const = 0;
};

class CShaderConstantParam final
{
private:

	PShaderConstantInfo _Info;
	U32                 _Offset;

	CShaderConstantParam(PShaderConstantInfo Info, U32 Offset);

public:

	CShaderConstantParam(PShaderConstantInfo Info);

	CStrID GetID() const { return _Info ? _Info->GetID() : CStrID::Empty; }
	bool   IsValid() const { return _Info.IsValidPtr(); }

	void   SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const;

	void   SetFloat(CConstantBuffer& CB, float Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, &Value, 1); }
	//void   SetInt(CConstantBuffer& CB, I32 Value) const;
	//void   SetUInt(CConstantBuffer& CB, U32 Value) const;
	//void   SetBool(CConstantBuffer& CB, bool Value) const;
	//void   SetVector(CConstantBuffer& CB, const vector3& Value) const;
	//void   SetVector(CConstantBuffer& CB, const vector4& Value) const;
	//void   SetMatrix(CConstantBuffer& CB, const matrix44& Value) const;

	void   SetFloatArray(CConstantBuffer& CB, const float* pValues, UPTR Count, U32 StartIndex = 0) const;
	void   SetFloatArray(CConstantBuffer& CB, std::initializer_list<float> Values, U32 StartIndex = 0) const { SetFloatArray(CB, Values.begin(), Values.size(), StartIndex); }
	//void   SetIntArray(CConstantBuffer& CB, const I32* pValues, UPTR Count, U32 StartIndex = 0) const;
	//void   SetUIntArray(CConstantBuffer& CB, const U32* pValues, UPTR Count, U32 StartIndex = 0) const;
	//void   SetBoolArray(CConstantBuffer& CB, const bool* pValues, UPTR Count, U32 StartIndex = 0) const;
	//void   SetVectorArray(CConstantBuffer& CB, const vector3* pValues, UPTR Count, U32 StartIndex = 0) const;
	//void   SetVectorArray(CConstantBuffer& CB, const vector4* pValues, UPTR Count, U32 StartIndex = 0) const;
	//void   SetMatrixArray(CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex = 0) const;

	CShaderConstantParam GetMember(const char* pName) const;
	CShaderConstantParam GetElement(U32 Index) const;
	CShaderConstantParam GetComponent(U32 Index) const;

	CShaderConstantParam x() const { return GetComponent(0); }
	CShaderConstantParam y() const { return GetComponent(1); }
	CShaderConstantParam z() const { return GetComponent(2); }
	CShaderConstantParam w() const { return GetComponent(3); }

	CShaderConstantParam operator [](const char* pName) const { return GetMember(pName); }
	CShaderConstantParam operator [](U32 Index) const { return GetElement(Index); }
	CShaderConstantParam operator ()(U32 Row, U32 Column) const;
	CShaderConstantParam operator ()(U32 ComponentIndex) const;
};

class IShaderConstantParam : public Data::CRefCounted
{
protected:

	virtual U32  GetMemberOffset(const char* pName) const = 0;
	virtual U32  GetElementOffset(U32 Index) const = 0;
	virtual U32  GetComponentOffset(U32 Index) const = 0;

	virtual U32  GetRowCount() const = 0;
	virtual U32  GetColumnCount() const = 0;
	virtual bool IsColumnMajor() const = 0;

public:

	virtual CStrID                GetID() const = 0;
	virtual IConstantBufferParam& GetConstantBuffer() const = 0;

	//!!!TODO: member, element and component operations! operator[int/str] + methods like .x()?
	//!!!TODO: matrix majority!

	virtual void SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const = 0;
	virtual void SetFloats(CConstantBuffer& CB, const float* pValue, UPTR Count) const = 0;
	virtual void SetInts(CConstantBuffer& CB, const I32* pValue, UPTR Count) const = 0;
	virtual void SetUInts(CConstantBuffer& CB, const U32* pValue, UPTR Count) const = 0;
	virtual void SetBools(CConstantBuffer& CB, const bool* pValue, UPTR Count) const = 0;

	void         SetFloat(CConstantBuffer& CB, float Value) const { SetFloats(CB, &Value, 1); }
	void         SetInt(CConstantBuffer& CB, I32 Value) const { SetInts(CB, &Value, 1); }
	void         SetUInt(CConstantBuffer& CB, U32 Value) const { SetUInts(CB, &Value, 1); }
	void         SetBool(CConstantBuffer& CB, bool Value) const { SetBools(CB, &Value, 1); }
	void         SetVector(CConstantBuffer& CB, const vector3& Value) const { SetFloats(CB, Value.v, 3); }
	void         SetVector(CConstantBuffer& CB, const vector4& Value) const { SetFloats(CB, Value.v, 4); }
	void         SetVectors(CConstantBuffer& CB, const vector4* pValue, UPTR Count) const { SetFloats(CB, pValue->v, 4 * Count); }
	void         SetMatrix(CConstantBuffer& CB, const matrix44& Value) const { SetMatrices(CB, &Value, 1); }
	void         SetMatrices(CConstantBuffer& CB, const matrix44* pValue, UPTR Count) const;
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
	std::vector<CShaderConstantParam> _Constants;
	std::vector<PConstantBufferParam> _ConstantBuffers;
	std::vector<PResourceParam>       _Resources;
	std::vector<PSamplerParam>        _Samplers;

public:

	CShaderParamTable(std::vector<CShaderConstantParam>&& Constants,
		std::vector<PConstantBufferParam>&& ConstantBuffers,
		std::vector<PResourceParam>&& Resources,
		std::vector<PSamplerParam>&& Samplers);

	size_t                GetConstantIndex(CStrID ID) const;
	size_t                GetConstantBufferIndex(CStrID ID) const;
	size_t                GetConstantBufferIndexForConstant(CStrID ID) const;
	size_t                GetConstantBufferIndexForConstant(size_t Index) const;
	size_t                GetResourceIndex(CStrID ID) const;
	size_t                GetSamplerIndex(CStrID ID) const;

	IShaderConstantParam* GetConstant(size_t Index) const;
	IConstantBufferParam* GetConstantBuffer(size_t Index) const;
	IResourceParam*       GetResource(size_t Index) const;
	ISamplerParam*        GetSampler(size_t Index) const;

	IShaderConstantParam* GetConstant(CStrID ID) const { return GetConstant(GetConstantIndex(ID)); }
	IConstantBufferParam* GetConstantBuffer(CStrID ID) const { return GetConstantBuffer(GetConstantBufferIndex(ID)); }
	IResourceParam*       GetResource(CStrID ID) const { return GetResource(GetResourceIndex(ID)); }
	ISamplerParam*        GetSampler(CStrID ID) const { return GetSampler(GetSamplerIndex(ID)); }

	const auto&           GetConstants() const { return _Constants; }
	const auto&           GetConstantBuffers() const { return _ConstantBuffers; }
	const auto&           GetResources() const { return _Resources; }
	const auto&           GetSamplers() const { return _Samplers; }
};

}
