#include "ShaderParamTable.h"
#include <algorithm>

namespace Render
{
__ImplementClassNoFactory(IConstantBufferParam, Core::CObject);

CShaderConstantInfo::~CShaderConstantInfo() = default;
//---------------------------------------------------------------------

PShaderConstantInfo CShaderConstantInfo::GetMemberInfo(CStrID Name)
{
	// An array, can't access members even if array element is a structure
	if (ElementCount > 1) return nullptr;

	// Not a structure
	const auto MemberCount = GetMemberCount();
	if (!MemberCount) return nullptr;

	const auto MemberIndex = GetMemberIndex(Name);

	// Has no member with requested Name
	if (MemberIndex >= MemberCount) return nullptr;

	if (SubInfo)
	{
		// Cache is mapped to struct members, check at member index
		if (SubInfo[MemberIndex]) return SubInfo[MemberIndex];
	}
	else
	{
		// Member cache is not created yet, allocate
		SubInfo = std::make_unique<PShaderConstantInfo[]>(MemberCount);
	}

	SubInfo[MemberIndex] = Struct.Members[MemberIndex]->Clone();
	// patch fields!

	// find member
	// if not found in structure, return nullptr;
	// create mutable cached pointer
	// return member
	return SubInfo[MemberIndex];
}
//---------------------------------------------------------------------

PShaderConstantInfo CShaderConstantInfo::GetElementInfo()
{
	// Not an array, is element itself 
	if (ElementCount < 2) return this;

	// If cache is valid, there is the only element there
	if (SubInfo) return SubInfo[0];

	SubInfo = std::make_unique<PShaderConstantInfo[]>(1);
	SubInfo[0] = Clone();
	SubInfo[0]->ElementCount = 1;

	return SubInfo[0];
}
//---------------------------------------------------------------------

PShaderConstantInfo CShaderConstantInfo::GetRowInfo()
{
	// An array, can't access components
	if (ElementCount > 1) return nullptr;

	// Don't check if it is a structure, because structure must have MajorDim = 1
	n_assert_dbg(!GetMemberCount());

	// Not a matrix (2-dimensional object), has no rows
	const auto MajorDim = IsColumnMajor() ? Columns : Rows;
	if (MajorDim < 2) return nullptr;

	// If cache is valid, there are component at [0] and row at [1]
	if (SubInfo)
	{
		if (SubInfo[1]) return SubInfo[1];
	}
	else
	{
		SubInfo = std::make_unique<PShaderConstantInfo[]>(2);
	}

	SubInfo[1] = Clone();
	if (IsColumnMajor())
		SubInfo[1]->Columns = 1;
	else
		SubInfo[1]->Rows = 1;

	//!!!fix stride!

	return SubInfo[1];
}
//---------------------------------------------------------------------

PShaderConstantInfo CShaderConstantInfo::GetComponentInfo()
{
	// An array, can't access components
	if (ElementCount > 1) return nullptr;

	// A structure, can't access components
	if (GetMemberCount()) return nullptr;

	// Single component constant is a component itself
	if (Rows < 2 && Columns < 2) return this;

	// If cache is valid, there is component at [0]
	if (SubInfo)
	{
		if (SubInfo[0]) return SubInfo[0];
	}
	else
	{
		// If it is a matrix, allocate slot for the row info too
		const auto MajorDim = IsColumnMajor() ? Columns : Rows;
		SubInfo = std::make_unique<PShaderConstantInfo[]>((MajorDim > 1) ? 2 : 1);
	}

	SubInfo[0] = Clone();
	SubInfo[0]->Columns = 1;
	SubInfo[0]->Rows = 1;

	//!!!fix stride!

	return SubInfo[1];
}
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
	if (!_Info) return CShaderConstantParam(nullptr, 0);
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
	//???!!!recalc into row-column with majority and call that method?

	if (!_Info ||
		_Info->GetElementCount() > 1 ||
		_Info->GetMemberCount() > 0 ||
		_Info->GetRowCount() * _Info->GetColumnCount() <= Index)
	{
		return CShaderConstantParam(nullptr, 0);
	}

	//???is component stride enough or for matrix3x2 there will be padding between rows?
	return CShaderConstantParam(_Info->GetComponentInfo(), _Offset + Index * _Info->GetComponentStride());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetComponent(U32 Row, U32 Column) const
{
	if (!_Info ||
		_Info->GetElementCount() > 1 ||
		_Info->GetMemberCount() > 0 ||
		_Info->GetRowCount() <= Row ||
		_Info->GetColumnCount() <= Column)
	{
		return CShaderConstantParam(nullptr, 0);
	}

	//???is component stride enough or for matrix3x2 there will be padding between rows?
	const auto Index = _Info->IsColumnMajor() ?
		Row * _Info->GetColumnCount() + Column :
		Column * _Info->GetRowCount() + Row;

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
