#include "ShaderParamTable.h"
#include <algorithm>

namespace Render
{
__ImplementClassNoFactory(IConstantBufferParam, Core::CObject);

CShaderConstantInfo::CShaderConstantInfo(size_t ConstantBufferIndex, const CShaderConstantMeta& Meta)
	: _CBIndex(ConstantBufferIndex)
	, _Meta(Meta)
{
}
//---------------------------------------------------------------------

CShaderConstantInfo::~CShaderConstantInfo() = default;
//---------------------------------------------------------------------

// Create sub-constant with offset precalculated in the parent
CShaderConstantParam::CShaderConstantParam(PShaderConstantInfo Info, U32 Offset)
	: _Info(Info)
	, _Offset(Offset)
{
}
//---------------------------------------------------------------------

// Create top-level constant
CShaderConstantParam::CShaderConstantParam(PShaderConstantInfo Info)
	: _Info(Info)
	, _Offset(Info ? Info->GetLocalOffset() : 0)
{
}
//---------------------------------------------------------------------

template<typename T>
static inline void SetValues(CShaderConstantInfo* Info, CConstantBuffer& CB, U32 Offset, const T* pValues, UPTR Count)
{
	if constexpr (std::is_same<T, float>())
		Info->SetFloats(CB, Offset, pValues, Count);
	else if constexpr (std::is_same<T, I32>())
		Info->SetInts(CB, Offset, pValues, Count);
	else if constexpr (std::is_same<T, U32>())
		Info->SetUInts(CB, Offset, pValues, Count);
	else if constexpr (std::is_same<T, bool>())
		Info->SetBools(CB, Offset, pValues, Count);
	else
		static_assert(false, "Unsupported type in shader constant SetValues");
}
//---------------------------------------------------------------------

template<typename T>
static inline void SetArray(CShaderConstantInfo* Info, CConstantBuffer& CB, U32 Offset, const T* pValues, UPTR Count, U32 StartIndex)
{
	n_assert_dbg(Info);
	if (!pValues || !Info || StartIndex >= Info->GetElementCount()) return;

	Count = std::min(Count, Info->GetElementCount() - StartIndex);
	if (!Count) return;

	Offset += StartIndex * Info->GetElementStride();

	if (Info->HasElementPadding() || Info->NeedConversionFrom(/*float*/))
	{
		const auto* pEnd = pValues + Count;
		for (; pValues < pEnd; ++pValues)
		{
			SetValues(Info, CB, Offset, pValues, 1);
			Offset += Info->GetElementStride();
		}
	}
	else
	{
		SetValues(Info, CB, Offset, pValues, Count);
	}
}
//---------------------------------------------------------------------

void CShaderConstantParam::SetFloatArray(CConstantBuffer& CB, const float* pValues, UPTR Count, U32 StartIndex) const
{
	SetArray(_Info, CB, _Offset, pValues, Count, StartIndex);
}
//---------------------------------------------------------------------

void CShaderConstantParam::SetIntArray(CConstantBuffer& CB, const I32* pValues, UPTR Count, U32 StartIndex) const
{
	SetArray(_Info, CB, _Offset, pValues, Count, StartIndex);
}
//---------------------------------------------------------------------

void CShaderConstantParam::SetUIntArray(CConstantBuffer& CB, const U32* pValues, UPTR Count, U32 StartIndex) const
{
	SetArray(_Info, CB, _Offset, pValues, Count, StartIndex);
}
//---------------------------------------------------------------------

void CShaderConstantParam::SetBoolArray(CConstantBuffer& CB, const bool* pValues, UPTR Count, U32 StartIndex) const
{
	SetArray(_Info, CB, _Offset, pValues, Count, StartIndex);
}
//---------------------------------------------------------------------

void CShaderConstantParam::InternalSetMatrix(CConstantBuffer& CB, const matrix44& Value) const
{
	const auto MajorDim = _Info->IsColumnMajor() ? _Info->GetColumnCount() : _Info->GetRowCount();
	const auto MinorDim = _Info->IsColumnMajor() ? _Info->GetRowCount() : _Info->GetColumnCount();
	if (MajorDim == 4)
		_Info->SetFloats(CB, _Offset, *Value.m, MajorDim * MinorDim);
	else
	{
		U32 Offset = _Offset;
		for (size_t Min = 0; Min < MinorDim; ++Min, Offset += 4 * sizeof(float))
			_Info->SetFloats(CB, Offset, *Value.m, MajorDim);
	}
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetMember(CStrID Name) const
{
	if (!_Info || _Info->GetElementCount() > 1) return CShaderConstantParam(nullptr, 0);
	return CShaderConstantParam(_Info->GetMemberInfo(Name), _Offset + _Info->GetLocalOffset());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetElement(U32 Index) const
{
	if (!_Info || _Info->GetElementCount() <= Index) return CShaderConstantParam(nullptr, 0);
	return CShaderConstantParam(_Info->GetElementInfo(), _Offset + Index * _Info->GetElementStride());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetComponent(U32 Index) const
{
	if (!_Info ||
		_Info->GetElementCount() > 1 ||
		_Info->GetMemberCount() > 0 ||
		_Info->GetRowCount() * _Info->GetColumnCount <= Index)
	{
		return CShaderConstantParam(nullptr, 0);
	}

	return CShaderConstantParam(_Info->GetComponentInfo(), _Offset + Index * _Info->GetComponentStride());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetComponent(U32 Row, U32 Column) const
{
	if (!_Info ||
		_Info->GetElementCount() > 1 ||
		_Info->GetMemberCount() > 0 ||
		_Info->GetRowCount() <= Row ||
		_Info->GetColumnCount <= Column)
	{
		return CShaderConstantParam(nullptr, 0);
	}

	//???must index honour majority?

	return CShaderConstantParam(_Info->GetComponentInfo(), _Offset + Index * _Info->GetComponentStride());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::operator [](U32 Index) const
{
	if (!_Info) return CShaderConstantParam(nullptr, 0);

	if (_Info->GetElementCount() > 1) return GetElement(Index);

	const auto MajorDim = _Info->IsColumnMajor() ? _Info->GetColumnCount() : _Info->GetRowCount();
	//!!!if (MajorDim > 1) return GetRow(Index);

	const auto MinorDim = _Info->IsColumnMajor() ? _Info->GetRowCount() : _Info->GetColumnCount();
	if (MinorDim) return GetComponent(Index);

	return CShaderConstantParam(nullptr, 0);
}
//---------------------------------------------------------------------

CShaderParamTable::CShaderParamTable(std::vector<CShaderConstantParam>&& Constants,
	std::vector<PConstantBufferParam>&& ConstantBuffers,
	std::vector<PResourceParam>&& Resources,
	std::vector<PSamplerParam>&& Samplers)

	: _Constants(std::move(Constants))
	, _ConstantBuffers(std::move(ConstantBuffers))
	, _Resources(std::move(Resources))
	, _Samplers(std::move(Samplers))
{
	std::sort(_Constants.begin(), _Constants.end(), [](const CShaderConstantParam& a, const CShaderConstantParam& b)
	{
		return a.GetID() < b.GetID();
	});
	std::sort(_ConstantBuffers.begin(), _ConstantBuffers.end(), [](const PConstantBufferParam& a, const PConstantBufferParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Resources.begin(), _Resources.end(), [](const PResourceParam& a, const PResourceParam& b)
	{
		return a->GetID() < b->GetID();
	});
	std::sort(_Samplers.begin(), _Samplers.end(), [](const PSamplerParam& a, const PSamplerParam& b)
	{
		return a->GetID() < b->GetID();
	});
}
//---------------------------------------------------------------------

}
