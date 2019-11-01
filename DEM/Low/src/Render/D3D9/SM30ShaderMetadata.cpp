#include "SM30ShaderMetadata.h"
#include <Render/D3D9/D3D9GPUDriver.h>
#include <Render/D3D9/D3D9ConstantBuffer.h>
#include <Render/D3D9/D3D9Texture.h>
#include <Render/D3D9/D3D9Sampler.h>

#undef min
#undef max

namespace Render
{
__ImplementClassNoFactory(CSM30ConstantBufferParam, IConstantBufferParam);

template<typename TTo>
static inline TTo* Cast(Core::CRTTIBaseClass& Value)
{
#if DEM_SHADER_META_DYNAMIC_TYPE_VALIDATION
	return Value.As<TTo>();
#else
	n_assert_dbg(Value.IsA<TTo>());
	return static_cast<TTo*>(&Value);
#endif
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const
{
}
//---------------------------------------------------------------------

CSM30ConstantParam::CSM30ConstantParam(PSM30ConstantBufferParam Buffer, PSM30ConstantMeta Meta, ESM30RegisterSet RegisterSet, U32 OffsetInBytes)
	: _Buffer(Buffer)
	, _Meta(Meta)
	, _RegisterSet(RegisterSet)
{
	n_assert_dbg(_Buffer && _RegisterSet != Reg_Invalid);

	CSM30ConstantBufferParam::CRanges* pRanges;
	U32 BytesPerRegister;
	switch (_RegisterSet)
	{
		case Reg_Float4:
		{
			pRanges = &_Buffer->Float4;
			BytesPerRegister = sizeof(float) * 4;
			break;
		}
		case Reg_Int4:
		{
			pRanges = &_Buffer->Int4;
			BytesPerRegister = sizeof(I32) * 4;
			break;
		}
		case Reg_Bool:
		{
			pRanges = &_Buffer->Bool;
			BytesPerRegister = sizeof(BOOL);
			break;
		}
		default:
		{
			::Sys::Error("CSM30ConstantParam() > invalid register set");
			return;
		}
	};

	U32 OffsetInRegisters = 0;

	for (const auto& Range : *pRanges)
	{
		// Ranges are sorted ascending, so that means that the buffer has no required range
		if (Range.first > _Meta->RegisterStart) break;

		if (Range.first + Range.second > _Meta->RegisterStart)
		{
			// Found range
			n_assert_dbg(Range.first + Range.second >= _Meta->RegisterStart + _Meta->ElementRegisterCount * _Meta->ElementCount);
			OffsetInRegisters += _Meta->RegisterStart - Range.first;
			_OffsetInBytes = OffsetInRegisters * BytesPerRegister + OffsetInBytes;
			_SizeInBytes = _Meta->ElementCount * _Meta->ElementRegisterCount * BytesPerRegister;
			return;
		}

		OffsetInRegisters += Range.second;
	}

	::Sys::Error("CSM30ConstantParam() > provided buffer doesn't contain the constant");
}
//---------------------------------------------------------------------

U32 CSM30ConstantParam::GetMemberOffset(const char* pName) const
{
	n_assert(_Meta->ElementCount == 1);
	return static_cast<U32>(-1);
}
//---------------------------------------------------------------------

U32 CSM30ConstantParam::GetElementOffset(U32 Index) const
{
	U32 BytesPerRegister;
	switch (_RegisterSet)
	{
		case Reg_Float4: BytesPerRegister = sizeof(float) * 4; break;
		case Reg_Int4:   BytesPerRegister = sizeof(I32) * 4; break;
		case Reg_Bool:   BytesPerRegister = sizeof(BOOL); break;
		default:         ::Sys::Error("CSM30ConstantParam::GetElementOffset() > invalid register set"); return;
	};
	return _OffsetInBytes + Index * _Meta->ElementRegisterCount * BytesPerRegister;
}
//---------------------------------------------------------------------

U32 CSM30ConstantParam::GetComponentOffset(U32 Index) const
{
	/*
	//???process column-major differently?
	n_assert_dbg(StructHandle == INVALID_HANDLE);

	const U32 ComponentsPerElement = Columns * Rows;
	const U32 Elm = ComponentIndex / ComponentsPerElement;
	ComponentIndex = ComponentIndex - Elm * ComponentsPerElement;
	const U32 Row = ComponentIndex / Columns;
	const U32 Col = ComponentIndex - Row * Columns;

	const U32 ComponentSize = 4; // Always 32-bit, even bool
	const U32 ComponentsPerAlignedRow = 4; // Even for, say, float3x3, each row uses full 4-component register

	return Offset + Elm * ComponentsPerElement + Row * ComponentsPerAlignedRow + Col; // In register components
	*/

	return static_cast<U32>(-1);
}
//---------------------------------------------------------------------


IConstantBufferParam& CSM30ConstantParam::GetConstantBuffer() const
{
	return *_Buffer;
}
//---------------------------------------------------------------------

void CSM30ConstantParam::SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const
{
	if (!pValue || !Size) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
		pCB->WriteData(_RegisterSet, _OffsetInBytes, pValue, std::min(Size, _SizeInBytes));
}
//---------------------------------------------------------------------

void CSM30ConstantParam::SetFloats(CConstantBuffer& CB, const float* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		if (_RegisterSet == Reg_Float4)
		{
			pCB->WriteData(Reg_Float4, _OffsetInBytes, pValue, std::min(Count * sizeof(float), _SizeInBytes));
		}
		else if (_RegisterSet == Reg_Int4)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount * 4);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const I32 Value = static_cast<I32>(*pValue);
				pCB->WriteData(Reg_Int4, Offset, &Value, sizeof(I32));
				++pValue;
				Offset += sizeof(I32);
			}
		}
		else if (_RegisterSet == Reg_Bool)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const BOOL Value = (*pValue != 0.f);
				pCB->WriteData(Reg_Bool, Offset, &Value, sizeof(BOOL));
				++pValue;
				Offset += sizeof(BOOL);
			}
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantParam::SetInts(CConstantBuffer& CB, const I32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		if (_RegisterSet == Reg_Int4)
		{
			pCB->WriteData(Reg_Int4, _OffsetInBytes, pValue, std::min(Count * sizeof(I32), _SizeInBytes));
		}
		else if (_RegisterSet == Reg_Float4)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount * 4);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const float Value = static_cast<float>(*pValue);
				pCB->WriteData(Reg_Float4, Offset, &Value, sizeof(float));
				++pValue;
				Offset += sizeof(float);
			}
		}
		else if (_RegisterSet == Reg_Bool)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const BOOL Value = (*pValue != 0);
				pCB->WriteData(Reg_Bool, Offset, &Value, sizeof(BOOL));
				++pValue;
				Offset += sizeof(BOOL);
			}
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantParam::SetUInts(CConstantBuffer& CB, const U32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		if (_RegisterSet == Reg_Int4)
		{
			pCB->WriteData(Reg_Int4, _OffsetInBytes, pValue, std::min(Count * sizeof(U32), _SizeInBytes));
		}
		else if (_RegisterSet == Reg_Float4)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount * 4);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const float Value = static_cast<float>(*pValue);
				pCB->WriteData(Reg_Float4, Offset, &Value, sizeof(float));
				++pValue;
				Offset += sizeof(float);
			}
		}
		else if (_RegisterSet == Reg_Bool)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const BOOL Value = (*pValue != 0);
				pCB->WriteData(Reg_Bool, Offset, &Value, sizeof(BOOL));
				++pValue;
				Offset += sizeof(BOOL);
			}
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantParam::SetBools(CConstantBuffer& CB, const bool* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		auto Offset = _OffsetInBytes;

		if (_RegisterSet == Reg_Float4)
		{
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount * 4);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const float Value = *pValue ? 1.f : 0.f;
				pCB->WriteData(Reg_Float4, Offset, &Value, sizeof(float));
				++pValue;
				Offset += sizeof(float);
			}
		}
		else if (_RegisterSet == Reg_Int4)
		{
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount * 4);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const I32 Value = *pValue ? 1 : 0;
				pCB->WriteData(Reg_Float4, Offset, &Value, sizeof(I32));
				++pValue;
				Offset += sizeof(I32);
			}
		}
		else if (_RegisterSet == Reg_Bool)
		{
			Count = std::min(Count, _Meta->ElementCount * _Meta->ElementRegisterCount);
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const BOOL Value = *pValue ? 1 : 0;
				pCB->WriteData(Reg_Bool, Offset, &Value, sizeof(BOOL));
				++pValue;
				Offset += sizeof(BOOL);
			}
		}
	}
}
//---------------------------------------------------------------------

bool CSM30ConstantBufferParam::Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const
{
	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pCB = pValue ? Cast<CD3D9ConstantBuffer>(*pValue) : nullptr;
	if (pValue && !pCB) FAIL;

	if (ShaderTypeMask & ShaderType_Vertex)
	{
		if (!pGPU->BindConstantBuffer(ShaderType_Vertex, SlotIndex, pCB)) FAIL;
	}

	if (ShaderTypeMask & ShaderType_Pixel)
	{
		if (!pGPU->BindConstantBuffer(ShaderType_Pixel, SlotIndex, pCB)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CSM30ConstantBufferParam::IsBufferCompatible(CConstantBuffer& Value) const
{
	const auto* pCB = Cast<CD3D9ConstantBuffer>(Value);
	return pCB && this == pCB->GetMetadata();
}
//---------------------------------------------------------------------

bool CSM30ResourceParam::Apply(CGPUDriver& GPU, CTexture* pValue) const
{
	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pTex = pValue ? Cast<CD3D9Texture>(*pValue) : nullptr;
	if (pValue && !pTex) FAIL;

	if (_ShaderTypeMask & ShaderType_Vertex)
	{
		if (!pGPU->BindResource(ShaderType_Vertex, _Register, pTex)) FAIL;
	}

	if (_ShaderTypeMask & ShaderType_Pixel)
	{
		if (!pGPU->BindResource(ShaderType_Pixel, _Register, pTex)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CSM30SamplerParam::Apply(CGPUDriver& GPU, CSampler* pValue) const
{
	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pSampler = pValue ? Cast<CD3D9Sampler>(*pValue) : nullptr;
	if (pValue && !pSampler) FAIL;

	if (_ShaderTypeMask & ShaderType_Vertex)
	{
		if (!pGPU->BindSampler(ShaderType_Vertex, _RegisterStart, _RegisterCount, pSampler)) FAIL;
	}

	if (_ShaderTypeMask & ShaderType_Pixel)
	{
		if (!pGPU->BindSampler(ShaderType_Pixel, _RegisterStart, _RegisterCount, pSampler)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

}
