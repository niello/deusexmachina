#pragma once
#include <Render/RenderFwd.h>
#include <Core/Object.h>
#include <map>

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

enum EShaderConstantFlags : U8
{
	ColumnMajor = 0x01
};

class CShaderConstantInfo : public Core::CObject
{
protected:

	// Cached sub-constant info. Any array has elements. Any single structure has members.
	// Any single vector has components. Any single matrix has components at [0] and rows at [1].
	// Member info is a sorted array mapped to struct members array. It could be a union,
	// but it makes default constructor deleted.
	std::unique_ptr<PShaderConstantInfo[]> SubInfo;

	virtual PShaderConstantInfo Clone() const = 0;

public:

	PShaderStructureInfo Struct;
	size_t               BufferIndex;
	CStrID               Name;
	U32                  LocalOffset;
	U32                  ElementStride;
	U32                  ElementCount;
	U32                  ComponentStride;
	U8                   Rows;
	U8                   Columns;
	U8                   Flags;

	virtual ~CShaderConstantInfo() override;

	virtual void SetRawValue(CConstantBuffer& CB, U32 Offset, const void* pValue, UPTR Size) const = 0;
	virtual void SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const = 0;
	virtual void SetInts(CConstantBuffer& CB, U32 Offset, const I32* pValue, UPTR Count) const = 0;
	virtual void SetUInts(CConstantBuffer& CB, U32 Offset, const U32* pValue, UPTR Count) const = 0;
	virtual void SetBools(CConstantBuffer& CB, U32 Offset, const bool* pValue, UPTR Count) const = 0;

	CStrID GetID() const { return Name; }
	size_t GetConstantBufferIndex() const { return BufferIndex; }
	U32    GetLocalOffset() const { return LocalOffset; }
	U32    GetElementStride() const { return ElementStride; }
	U32    GetElementCount() const { return ElementCount; }
	U32    GetComponentStride() const { return ComponentStride; }
	U32    GetRowCount() const { return Rows; }
	U32    GetColumnCount() const { return Columns; }
	bool   IsColumnMajor() const { return Flags & EShaderConstantFlags::ColumnMajor; }
	bool   HasElementPadding() const;
	bool   NeedConversionFrom(/*type*/) const;

	PShaderConstantInfo GetMemberInfo(CStrID Name);
	PShaderConstantInfo GetElementInfo();
	PShaderConstantInfo GetRowInfo();
	PShaderConstantInfo GetComponentInfo();
};

class CShaderStructureInfo : public Data::CRefCounted
{
public:

	//!!!members must be sorted by CStrID ID!

	//CStrID Name;
	std::vector<PShaderConstantInfo> Members;
};

class CShaderConstantParam final
{
private:

	PShaderConstantInfo _Info;
	U32                 _Offset = 0;

	CShaderConstantParam(PShaderConstantInfo Info, U32 Offset);

	void   InternalSetMatrix(CConstantBuffer& CB, const matrix44& Value) const;

public:

	CShaderConstantParam() = default;
	CShaderConstantParam(PShaderConstantInfo Info);

	CStrID GetID() const { return _Info ? _Info->GetID() : CStrID::Empty; }
	bool   IsValid() const { return _Info.IsValidPtr(); }
	size_t GetConstantBufferIndex() const { return _Info ? _Info->GetConstantBufferIndex() : InvalidParamIndex; }
	U32    GetElementCount() const { return _Info ? _Info->GetElementCount() : 0; }

	void   SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const { n_assert_dbg(_Info); if (_Info) _Info->SetRawValue(CB, _Offset, pValue, Size); }

	void   SetFloat(CConstantBuffer& CB, float Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, &Value, 1); }
	void   SetInt(CConstantBuffer& CB, I32 Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetInts(CB, _Offset, &Value, 1); }
	void   SetUInt(CConstantBuffer& CB, U32 Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetUInts(CB, _Offset, &Value, 1); }
	void   SetBool(CConstantBuffer& CB, bool Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetBools(CB, _Offset, &Value, 1); }
	void   SetVector(CConstantBuffer& CB, const vector3& Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, Value.v, 3); }
	void   SetVector(CConstantBuffer& CB, const vector4& Value) const { n_assert_dbg(_Info); if (_Info) _Info->SetFloats(CB, _Offset, Value.v, 4); }
	void   SetMatrix(CConstantBuffer& CB, const matrix44& Value, bool ColumnMajor = false) const { n_assert_dbg(_Info); if (_Info) { if (ColumnMajor == _Info->IsColumnMajor()) InternalSetMatrix(CB, Value); else InternalSetMatrix(CB, Value.transposed()); } }

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
	//void   SetMatrixArray(CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex = 0) const;

	CShaderConstantParam GetMember(CStrID Name) const;
	CShaderConstantParam GetElement(U32 Index) const;
	CShaderConstantParam GetComponent(U32 Index) const;
	CShaderConstantParam GetComponent(U32 Row, U32 Column) const;

	CShaderConstantParam X() const { return GetComponent(0); }
	CShaderConstantParam Y() const { return GetComponent(1); }
	CShaderConstantParam Z() const { return GetComponent(2); }
	CShaderConstantParam W() const { return GetComponent(3); }

	CShaderConstantParam operator [](const std::string_view& Name) const { return GetMember(CStrID(Name.data())); }
	CShaderConstantParam operator [](CStrID Name) const { return GetMember(Name); }
	CShaderConstantParam operator [](U32 Index) const;
	CShaderConstantParam operator ()(U32 Row, U32 Column) const { return GetComponent(Row, Column); }

	operator bool() const noexcept { return _Info.IsValidPtr(); }
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

	// Vectors are sorted by ID and never change in runtime
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
	size_t                GetResourceIndex(CStrID ID) const;
	size_t                GetSamplerIndex(CStrID ID) const;

	const CShaderConstantParam& GetConstant(size_t Index) const;
	IConstantBufferParam* GetConstantBuffer(size_t Index) const;
	IResourceParam*       GetResource(size_t Index) const;
	ISamplerParam*        GetSampler(size_t Index) const;

	const CShaderConstantParam& GetConstant(CStrID ID) const { return GetConstant(GetConstantIndex(ID)); }
	IConstantBufferParam* GetConstantBuffer(CStrID ID) const { return GetConstantBuffer(GetConstantBufferIndex(ID)); }
	IResourceParam*       GetResource(CStrID ID) const { return GetResource(GetResourceIndex(ID)); }
	ISamplerParam*        GetSampler(CStrID ID) const { return GetSampler(GetSamplerIndex(ID)); }

	const auto&           GetConstants() const { return _Constants; }
	const auto&           GetConstantBuffers() const { return _ConstantBuffers; }
	const auto&           GetResources() const { return _Resources; }
	const auto&           GetSamplers() const { return _Samplers; }
};

}
