#include "ShaderParamTable.h"
#include <algorithm>

namespace Render
{

const CShaderConstantParam CShaderConstantParam::Empty;

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
	else if constexpr (std::is_same<T, matrix44>())
		Info->SetFloats(CB, Offset, *pValues->m, Count * 16); // NB: only 4x4 matrices are supported here
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

	if (Info->GetElementStride() == Info->GetElementSize())
	{
		// Can write sequentially, there are no padding gaps
		SetValues(Info, CB, Offset, pValues, Count);
	}
	else
	{
		//Write elements one by one, skipping padding gaps
		const auto* pEnd = pValues + Count;
		for (; pValues < pEnd; ++pValues)
		{
			SetValues(Info, CB, Offset, pValues, 1);
			Offset += Info->GetElementStride();
		}
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

void CShaderConstantParam::SetMatrixArray(CConstantBuffer& CB, const matrix44* pValues, UPTR Count, U32 StartIndex, bool ColumnMajor) const
{
	n_assert_dbg(_Info);
	if (!_Info) return;

	const bool Transpose = (ColumnMajor != _Info->IsColumnMajor());
	const auto ComponentCount = _Info->GetColumnCount() * _Info->GetRowCount();

	if (!Transpose && ComponentCount == 16)
	{
		// Shader matrix array layout perfectly matches C++ matrix44[] layout
		SetArray(_Info, CB, _Offset, pValues, Count, StartIndex);
		return;
	}

	// Calculate metrics for source data
	const auto Rows = _Info->IsColumnMajor() ? _Info->GetColumnCount() : _Info->GetRowCount();
	const auto RowComponentCount = _Info->IsColumnMajor() ? _Info->GetRowCount() : _Info->GetColumnCount();

	auto Offset = _Offset + StartIndex * _Info->GetElementStride();
	const auto* pEnd = pValues + Count;

	if (RowComponentCount == 4)
	{
		// Write matrix by matrix
		if (Transpose)
		{
			for (; pValues < pEnd; ++pValues)
			{
				_Info->SetFloats(CB, Offset, *pValues->transposed().m, ComponentCount);
				Offset += _Info->GetElementStride();
			}
		}
		else
		{
			for (; pValues < pEnd; ++pValues)
			{
				_Info->SetFloats(CB, Offset, *pValues->m, ComponentCount);
				Offset += _Info->GetElementStride();
			}
		}
	}
	else
	{
		// Write row by row
		const UPTR RowBytes = RowComponentCount * sizeof(float);
		if (Transpose)
		{
			for (; pValues < pEnd; ++pValues)
			{
				const matrix44 Transposed = pValues->transposed();
				for (UPTR Row = 0; Row < Rows; ++Row)
					_Info->SetFloats(CB, Offset + Row * RowBytes, Transposed.m[Row], RowComponentCount);
				Offset += _Info->GetElementStride();
			}
		}
		else
		{
			for (; pValues < pEnd; ++pValues)
			{
				for (UPTR Row = 0; Row < Rows; ++Row)
					_Info->SetFloats(CB, Offset + Row * RowBytes, pValues->m[Row], RowComponentCount);
				Offset += _Info->GetElementStride();
			}
		}
	}
}
//---------------------------------------------------------------------

void CShaderConstantParam::InternalSetMatrix(CConstantBuffer& CB, const matrix44& Value) const
{
	const auto Rows = _Info->IsColumnMajor() ? _Info->GetColumnCount() : _Info->GetRowCount();
	const auto RowComponentCount = _Info->IsColumnMajor() ? _Info->GetRowCount() : _Info->GetColumnCount();
	if (RowComponentCount == 4)
	{
		_Info->SetFloats(CB, _Offset, *Value.m, RowComponentCount * Rows);
	}
	else
	{
		const UPTR RowBytes = RowComponentCount * sizeof(float);
		for (UPTR Row = 0; Row < Rows; ++Row)
			_Info->SetFloats(CB, _Offset + Row * RowBytes, Value.m[Row], RowComponentCount);
	}
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetMember(CStrID Name) const
{
	if (!_Info) return CShaderConstantParam();
	return CShaderConstantParam(_Info->GetMemberInfo(Name), _Offset + _Info->GetLocalOffset());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetElement(U32 Index) const
{
	if (!_Info || _Info->GetElementCount() <= Index) return CShaderConstantParam();
	return CShaderConstantParam(_Info->GetElementInfo(), _Offset + Index * _Info->GetElementStride());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetVector(U32 Index) const
{
	if (!_Info || _Info->Struct) return CShaderConstantParam();
	return CShaderConstantParam(_Info->GetVectorInfo(), _Offset + Index * _Info->GetVectorStride());
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetComponent(U32 Index) const
{
	if (!_Info || _Info->Struct) return CShaderConstantParam();

	const auto Rows = _Info->GetRowCount();
	const auto Cols = _Info->GetColumnCount();
	const auto ComponentCount = Rows * Cols;
	if (Index > ComponentCount)
	{
		return GetElement(Index / ComponentCount).GetComponent(Index % ComponentCount);
	}
	else
	{
		return _Info->IsColumnMajor() ?
			GetComponent(Index / Rows, Index % Rows) :
			GetComponent(Index % Cols, Index / Cols);
	}
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::GetComponent(U32 Row, U32 Column) const
{
	if (!_Info ||
		_Info->GetElementCount() > 1 ||
		_Info->Struct ||
		_Info->GetRowCount() <= Row ||
		_Info->GetColumnCount() <= Column)
	{
		return CShaderConstantParam();
	}

	if (_Info->IsColumnMajor())
	{
		const auto LocalOffset = Column * _Info->GetVectorStride() + Row * _Info->GetComponentSize();
		return CShaderConstantParam(_Info->GetComponentInfo(), _Offset + LocalOffset);
	}
	else
	{
		const auto LocalOffset = Row * _Info->GetVectorStride() + Column * _Info->GetComponentSize();
		return CShaderConstantParam(_Info->GetComponentInfo(), _Offset + LocalOffset);
	}
}
//---------------------------------------------------------------------

CShaderConstantParam CShaderConstantParam::operator [](U32 Index) const
{
	if (!_Info) return CShaderConstantParam();

	if (_Info->GetElementCount() > 1) return GetElement(Index);

	const auto MajorDim = _Info->IsColumnMajor() ? _Info->GetColumnCount() : _Info->GetRowCount();
	if (MajorDim > 1) return GetVector(Index);

	// FIXME: can use more lightweight GetComponent version here? Some calcs are already done!
	const auto MinorDim = _Info->IsColumnMajor() ? _Info->GetRowCount() : _Info->GetColumnCount();
	if (MinorDim) return GetComponent(Index);

	return CShaderConstantParam();
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
	_Constants.shrink_to_fit();
	_ConstantBuffers.shrink_to_fit();

	if (!_Constants.empty())
	{
		std::sort(_Constants.begin(), _Constants.end(), [](const CShaderConstantParam& a, const CShaderConstantParam& b)
		{
			return a.GetID() < b.GetID();
		});

		// Store constant to constant buffer mapping
		std::vector<IConstantBufferParam*> BufferMapping(_Constants.size());
		for (size_t i = 0; i < _Constants.size(); ++i)
			BufferMapping[i] = _ConstantBuffers[_Constants[i].GetConstantBufferIndex()];

		std::sort(_ConstantBuffers.begin(), _ConstantBuffers.end(), [](const PConstantBufferParam& a, const PConstantBufferParam& b)
		{
			return a->GetID() < b->GetID();
		});

		// Update constant buffer indices after sorting them
		for (size_t i = 0; i < _Constants.size(); ++i)
		{
			auto It = std::find(_ConstantBuffers.cbegin(), _ConstantBuffers.cend(), BufferMapping[i]);
			_Constants[i]._Info->BufferIndex = std::distance(_ConstantBuffers.cbegin(), It);
		}
	}

	_Resources.shrink_to_fit();
	std::sort(_Resources.begin(), _Resources.end(), [](const PResourceParam& a, const PResourceParam& b)
	{
		return a->GetID() < b->GetID();
	});

	_Samplers.shrink_to_fit();
	std::sort(_Samplers.begin(), _Samplers.end(), [](const PSamplerParam& a, const PSamplerParam& b)
	{
		return a->GetID() < b->GetID();
	});

	_HasParams = !_Constants.empty() || !_Resources.empty() || !_Samplers.empty();
}
//---------------------------------------------------------------------

size_t CShaderParamTable::GetConstantIndex(CStrID ID) const
{
	auto It = std::lower_bound(_Constants.cbegin(), _Constants.cend(), ID, [](const CShaderConstantParam& Obj, CStrID ID)
	{
		return Obj.GetID() < ID;
	});
	return (It != _Constants.cend() && (*It).GetID() == ID) ? std::distance(_Constants.cbegin(), It) : _Constants.size();
}
//---------------------------------------------------------------------

size_t CShaderParamTable::GetConstantBufferIndex(CStrID ID) const
{
	auto It = std::lower_bound(_ConstantBuffers.cbegin(), _ConstantBuffers.cend(), ID, [](const PConstantBufferParam& Obj, CStrID ID)
	{
		return Obj->GetID() < ID;
	});
	return (It != _ConstantBuffers.cend() && (*It)->GetID() == ID) ? std::distance(_ConstantBuffers.cbegin(), It) : _ConstantBuffers.size();
}
//---------------------------------------------------------------------

size_t CShaderParamTable::GetResourceIndex(CStrID ID) const
{
	auto It = std::lower_bound(_Resources.cbegin(), _Resources.cend(), ID, [](const PResourceParam& Obj, CStrID ID)
	{
		return Obj->GetID() < ID;
	});
	return (It != _Resources.cend() && (*It)->GetID() == ID) ? std::distance(_Resources.cbegin(), It) : _Resources.size();
}
//---------------------------------------------------------------------

size_t CShaderParamTable::GetSamplerIndex(CStrID ID) const
{
	auto It = std::lower_bound(_Samplers.cbegin(), _Samplers.cend(), ID, [](const PSamplerParam& Obj, CStrID ID)
	{
		return Obj->GetID() < ID;
	});
	return (It != _Samplers.cend() && (*It)->GetID() == ID) ? std::distance(_Samplers.cbegin(), It) : _Samplers.size();
}
//---------------------------------------------------------------------

}
