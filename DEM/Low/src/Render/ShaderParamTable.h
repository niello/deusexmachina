#pragma once
#include <Render/ShaderConstantInfo.h>
#include <Render/RenderFwd.h>

// Shader parameter table stores metadata necessary to set shader parameter values

#ifdef _DEBUG
#define DEM_SHADER_META_DYNAMIC_TYPE_VALIDATION (1)
#else
#define DEM_SHADER_META_DYNAMIC_TYPE_VALIDATION (0)
#endif

namespace Render
{
typedef Ptr<class CShaderStructureInfo> PShaderStructureInfo;

constexpr size_t InvalidParamIndex = std::numeric_limits<size_t>().max();

class CShaderConstantParam final
{
private:

	// For buffer index patching after sorting metadata by ID
	friend class CShaderParamTable;

	PShaderConstantInfo _Info;
	U32                 _Offset = 0;

	CShaderConstantParam(PShaderConstantInfo Info, U32 Offset);

	void   InternalSetMatrix(CConstantBuffer& CB, const matrix44& Value) const;

public:

	static const CShaderConstantParam Empty;

	CShaderConstantParam() = default;
	CShaderConstantParam(PShaderConstantInfo Info);

	CStrID GetID() const { return _Info ? _Info->GetID() : CStrID::Empty; }
	bool   IsValid() const { return _Info.IsValidPtr(); }
	size_t GetConstantBufferIndex() const { return _Info ? _Info->GetConstantBufferIndex() : InvalidParamIndex; }
	U32    GetElementCount() const { return _Info ? _Info->GetElementCount() : 0; }
	U32    GetElementStride() const { return _Info ? _Info->GetElementStride() : 0; }
	U32    GetTotalComponentCount() const { return _Info ? _Info->GetElementCount() * _Info->GetRowCount() * _Info->GetColumnCount() : 0; }

	// The cheapest possible way to switch between array elements, useful e.g. for setting member values in arrays of structures
	void   Shift(const CShaderConstantParam& ContainingArray, U32 Index) { _Offset = ContainingArray._Offset + Index * ContainingArray._Info->GetElementStride() + _Info->GetLocalOffset(); }

	void   SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const { n_assert_dbg(_Info); if (_Info) _Info->SetRawValue(CB, _Offset, pValue, Size); }

	void   SetFloat(CConstantBuffer& CB, float Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, &Value, 1); }
	void   SetInt(CConstantBuffer& CB, I32 Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetInts(CB, _Offset, &Value, 1); }
	void   SetUInt(CConstantBuffer& CB, U32 Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetUInts(CB, _Offset, &Value, 1); }
	void   SetBool(CConstantBuffer& CB, bool Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetBools(CB, _Offset, &Value, 1); }
	void   SetVector(CConstantBuffer& CB, const vector2& Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, Value.v, 2); }
	void   SetVector(CConstantBuffer& CB, const vector3& Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, Value.v, 3); }
	void   SetVector(CConstantBuffer& CB, const vector4& Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, Value.v, 4); }
	void   SetMatrix(CConstantBuffer& CB, const matrix44& Value, bool ColumnMajor = false) const { n_assert_dbg(_Info); if (_Info) { if (ColumnMajor == _Info->IsColumnMajor()) InternalSetMatrix(CB, Value); else InternalSetMatrix(CB, Value.transposed()); } }
	//!!!TODO: 16-float matrix setter for different math APIs!

	void   SetFloatArray(CConstantBuffer& CB, const float* pValues, UPTR Count, U32 StartIndex = 0) const;
	void   SetFloatArray(CConstantBuffer& CB, std::initializer_list<float> Values, U32 StartIndex = 0) const { SetFloatArray(CB, Values.begin(), Values.size(), StartIndex); }
	void   SetIntArray(CConstantBuffer& CB, const I32* pValues, UPTR Count, U32 StartIndex = 0) const;
	void   SetIntArray(CConstantBuffer& CB, std::initializer_list<I32> Values, U32 StartIndex = 0) const { SetIntArray(CB, Values.begin(), Values.size(), StartIndex); }
	void   SetUIntArray(CConstantBuffer& CB, const U32* pValues, UPTR Count, U32 StartIndex = 0) const;
	void   SetUIntArray(CConstantBuffer& CB, std::initializer_list<U32> Values, U32 StartIndex = 0) const { SetUIntArray(CB, Values.begin(), Values.size(), StartIndex); }
	void   SetBoolArray(CConstantBuffer& CB, const bool* pValues, UPTR Count, U32 StartIndex = 0) const;
	void   SetBoolArray(CConstantBuffer& CB, std::initializer_list<bool> Values, U32 StartIndex = 0) const { SetBoolArray(CB, Values.begin(), Values.size(), StartIndex); }
	//void   SetVectorArray(CConstantBuffer& CB, const vector3* pValues, UPTR Count, U32 StartIndex = 0) const;
	//void   SetVectorArray(CConstantBuffer& CB, const vector4* pValues, UPTR Count, U32 StartIndex = 0) const;
	void   SetMatrixArray(CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex = 0, bool ColumnMajor = false) const;

	CShaderConstantParam GetMember(CStrID Name) const;
	CShaderConstantParam GetElement(U32 Index) const;
	CShaderConstantParam GetVector(U32 Index) const;
	CShaderConstantParam GetComponent(U32 Index) const;
	CShaderConstantParam GetComponent(U32 Row, U32 Column) const;

	CShaderConstantParam X() const { return GetComponent(0); }
	CShaderConstantParam Y() const { return GetComponent(1); }
	CShaderConstantParam Z() const { return GetComponent(2); }
	CShaderConstantParam W() const { return GetComponent(3); }

	CShaderConstantParam operator [](const std::string_view& Name) const { return GetMember(CStrID(Name.data())); } //!!!FIXME: no need in CStrID for comparison!
	CShaderConstantParam operator [](CStrID Name) const { return GetMember(Name); }
	CShaderConstantParam operator [](U32 Index) const;
	CShaderConstantParam operator ()(U32 Row, U32 Column) const { return GetComponent(Row, Column); }
	CShaderConstantParam operator ()(U32 Index, const CShaderConstantParam& Member) const;

	operator bool() const noexcept { return _Info.IsValidPtr(); }
};

class IConstantBufferParam : public Core::CObject
{
	RTTI_CLASS_DECL(IConstantBufferParam, Core::CObject);

public:

	virtual CStrID GetID() const = 0;
	virtual bool   Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const = 0;
	virtual void   Unapply(CGPUDriver& GPU, CConstantBuffer* pValue) const = 0;
	virtual bool   IsBufferCompatible(CConstantBuffer& Value) const = 0;
};

class IResourceParam : public Data::CRefCounted
{
public:

	virtual CStrID GetID() const = 0;
	virtual bool   Apply(CGPUDriver& GPU, CTexture* pValue) const = 0;
	virtual void   Unapply(CGPUDriver& GPU, CTexture* pValue) const = 0;
};

class ISamplerParam : public Data::CRefCounted
{
public:

	virtual CStrID GetID() const = 0;
	virtual bool   Apply(CGPUDriver& GPU, CSampler* pValue) const = 0;
	virtual void   Unapply(CGPUDriver& GPU, CSampler* pValue) const = 0;
};

class CShaderParamTable : public Data::CRefCounted
{
protected:

	std::vector<CShaderConstantParam> _Constants;
	std::vector<PConstantBufferParam> _ConstantBuffers;
	std::vector<PResourceParam>       _Resources;
	std::vector<PSamplerParam>        _Samplers;
	bool                              _HasParams;

public:

	CShaderParamTable(std::vector<CShaderConstantParam>&& Constants,
		std::vector<PConstantBufferParam>&& ConstantBuffers,
		std::vector<PResourceParam>&& Resources,
		std::vector<PSamplerParam>&& Samplers);

	bool                        HasParams() const { return _HasParams; }

	size_t                      GetConstantIndex(CStrID ID) const;
	size_t                      GetConstantBufferIndex(CStrID ID) const;
	size_t                      GetResourceIndex(CStrID ID) const;
	size_t                      GetSamplerIndex(CStrID ID) const;

	const CShaderConstantParam& GetConstant(size_t Index) const { return Index < _Constants.size() ? _Constants[Index] : CShaderConstantParam::Empty; }
	IConstantBufferParam*       GetConstantBuffer(size_t Index) const { return Index < _ConstantBuffers.size() ? _ConstantBuffers[Index].Get() : nullptr; }
	IResourceParam*             GetResource(size_t Index) const { return Index < _Resources.size() ? _Resources[Index].Get() : nullptr; }
	ISamplerParam*              GetSampler(size_t Index) const { return Index < _Samplers.size() ? _Samplers[Index].Get() : nullptr; }

	const CShaderConstantParam& GetConstant(CStrID ID) const { return GetConstant(GetConstantIndex(ID)); }
	IConstantBufferParam*       GetConstantBuffer(CStrID ID) const { return GetConstantBuffer(GetConstantBufferIndex(ID)); }
	IResourceParam*             GetResource(CStrID ID) const { return GetResource(GetResourceIndex(ID)); }
	ISamplerParam*              GetSampler(CStrID ID) const { return GetSampler(GetSamplerIndex(ID)); }

	const auto&                 GetConstants() const { return _Constants; }
	const auto&                 GetConstantBuffers() const { return _ConstantBuffers; }
	const auto&                 GetResources() const { return _Resources; }
	const auto&                 GetSamplers() const { return _Samplers; }
};

}
