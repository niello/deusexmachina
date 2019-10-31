#include "USMShaderMetadata.h"
#include <Render/D3D11/D3D11GPUDriver.h>
#include <Render/D3D11/D3D11ConstantBuffer.h>
#include <Render/D3D11/D3D11Texture.h>
#include <Render/D3D11/D3D11Sampler.h>

namespace Render
{
__ImplementClassNoFactory(CUSMConstantBufferParam, IConstantBufferParam);

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

CUSMConstantParam::CUSMConstantParam(PUSMConstantBufferParam Buffer, PUSMConstantMeta Meta, U32 Offset)
	: _Buffer(Buffer)
	, _Meta(Meta)
	, _OffsetInBytes(Meta->Offset + Offset)
{
}
//---------------------------------------------------------------------

IConstantBufferParam& CUSMConstantParam::GetConstantBuffer() const
{
	return *_Buffer;
}
//---------------------------------------------------------------------

void CUSMConstantParam::SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const
{
	if (!pValue || !Size) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
		pCB->WriteData(_OffsetInBytes, pValue, std::min(Size, _Meta->ElementCount * _Meta->ElementStride));
}
//---------------------------------------------------------------------

void CUSMConstantParam::SetFloats(CConstantBuffer& CB, const float* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		const auto SizeInBytes = _Meta->ElementCount * _Meta->ElementStride;

		if (_Meta->Type == USMConst_Float)
		{
			pCB->WriteData(_OffsetInBytes, pValue, std::min(Count * sizeof(float), SizeInBytes));
		}
		else if (_Meta->Type == USMConst_Int || _Meta->Type == USMConst_Bool)
		{
			// NB: HLSL bool is a 32-bit int even in shader model 4.0+
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, SizeInBytes / sizeof(I32));
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const I32 Value = static_cast<I32>(*pValue);
				pCB->WriteData(Offset, &Value, sizeof(I32));
				++pValue;
				Offset += sizeof(I32);
			}
		}
		else ::Sys::Error("CUSMConstantParam() > typed value writing allowed only for float, int & bool constants");
	}
}
//---------------------------------------------------------------------

void CUSMConstantParam::SetInts(CConstantBuffer& CB, const I32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		const auto SizeInBytes = _Meta->ElementCount * _Meta->ElementStride;

		if (_Meta->Type == USMConst_Float)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, SizeInBytes / sizeof(float));
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const float Value = static_cast<float>(*pValue);
				pCB->WriteData(Offset, &Value, sizeof(float));
				++pValue;
				Offset += sizeof(float);
			}
		}
		else if (_Meta->Type == USMConst_Int || _Meta->Type == USMConst_Bool)
		{
			// NB: HLSL bool is a 32-bit int even in shader model 4.0+
			pCB->WriteData(_OffsetInBytes, pValue, std::min(Count * sizeof(I32), SizeInBytes));
		}
		else ::Sys::Error("CUSMConstantParam() > typed value writing allowed only for float, int & bool constants");
	}
}
//---------------------------------------------------------------------

void CUSMConstantParam::SetUInts(CConstantBuffer& CB, const U32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		const auto SizeInBytes = _Meta->ElementCount * _Meta->ElementStride;

		if (_Meta->Type == USMConst_Float)
		{
			auto Offset = _OffsetInBytes;
			Count = std::min(Count, SizeInBytes / sizeof(float));
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const float Value = static_cast<float>(*pValue);
				pCB->WriteData(Offset, &Value, sizeof(float));
				++pValue;
				Offset += sizeof(float);
			}
		}
		else if (_Meta->Type == USMConst_Int || _Meta->Type == USMConst_Bool)
		{
			// NB: HLSL bool is a 32-bit int even in shader model 4.0+
			pCB->WriteData(_OffsetInBytes, pValue, std::min(Count * sizeof(U32), SizeInBytes));
		}
		else ::Sys::Error("CUSMConstantParam() > typed value writing allowed only for float, int & bool constants");
	}
}
//---------------------------------------------------------------------

void CUSMConstantParam::SetBools(CConstantBuffer& CB, const bool* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		auto Offset = _OffsetInBytes;
		const auto SizeInBytes = _Meta->ElementCount * _Meta->ElementStride;

		if (_Meta->Type == USMConst_Float)
		{
			Count = std::min(Count, SizeInBytes / sizeof(float));
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const float Value = *pValue ? 1.f : 0.f;
				pCB->WriteData(Offset, &Value, sizeof(float));
				++pValue;
				Offset += sizeof(float);
			}
		}
		else if (_Meta->Type == USMConst_Int || _Meta->Type == USMConst_Bool)
		{
			// NB: HLSL bool is a 32-bit int even in sm4.0+
			Count = std::min(Count, SizeInBytes / sizeof(I32));
			const auto* pEnd = pValue + Count;
			while (pValue < pEnd)
			{
				const I32 Value = *pValue ? 1 : 0;
				pCB->WriteData(Offset, &Value, sizeof(I32));
				++pValue;
				Offset += sizeof(I32);
			}
		}
		else ::Sys::Error("CUSMConstantParam() > typed value writing allowed only for float, int & bool constants");
	}
}
//---------------------------------------------------------------------

bool CUSMConstantBufferParam::Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const
{
	auto pGPU = Cast<CD3D11GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pCB = pValue ? Cast<CD3D11ConstantBuffer>(*pValue) : nullptr;
	if (pValue && !pCB) FAIL;

	for (U8 i = 0; i < ShaderType_COUNT; ++i)
	{
		if (ShaderTypeMask & (1 << i))
		{
			if (!pGPU->BindConstantBuffer(static_cast<EShaderType>(i), Type, Register, pCB)) FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CUSMConstantBufferParam::IsBufferCompatible(CConstantBuffer& Value) const
{
	const auto* pCB = Cast<CD3D11ConstantBuffer>(Value);
	return pCB && Type == pCB->GetType() && Size <= pCB->GetSizeInBytes();
}
//---------------------------------------------------------------------

bool CUSMResourceParam::Apply(CGPUDriver& GPU, CTexture* pValue) const
{
	auto pGPU = Cast<CD3D11GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pTex = pValue ? Cast<CD3D11Texture>(*pValue) : nullptr;
	if (pValue && !pTex) FAIL;

	// FIXME: use _RegisterCount! Is for arrays?

	for (U8 i = 0; i < ShaderType_COUNT; ++i)
	{
		if (_ShaderTypeMask & (1 << i))
		{
			if (!pGPU->BindResource(static_cast<EShaderType>(i), _RegisterStart, pTex)) FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CUSMSamplerParam::Apply(CGPUDriver& GPU, CSampler* pValue) const
{
	auto pGPU = Cast<CD3D11GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pSampler = pValue ? Cast<CD3D11Sampler>(*pValue) : nullptr;
	if (pValue && !pSampler) FAIL;

	// FIXME: use _RegisterCount! Is for arrays?

	for (U8 i = 0; i < ShaderType_COUNT; ++i)
	{
		if (_ShaderTypeMask & (1 << i))
		{
			if (!pGPU->BindSampler(static_cast<EShaderType>(i), _RegisterStart, pSampler)) FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

}
