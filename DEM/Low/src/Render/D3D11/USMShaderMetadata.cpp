#include "USMShaderMetadata.h"
#include <Render/D3D11/D3D11GPUDriver.h>
#include <Render/D3D11/D3D11ConstantBuffer.h>
#include <Render/D3D11/D3D11Texture.h>
#include <Render/D3D11/D3D11Sampler.h>

// NB: HLSL bool is a 32-bit int even in shader model 4.0+

namespace Render
{
__ImplementClassNoFactory(CUSMConstantBufferParam, IConstantBufferParam);

template<typename T>
static inline T* Cast(Core::CRTTIBaseClass& Value)
{
#if DEM_SHADER_META_DYNAMICType_VALIDATION
	return Value.As<T>();
#else
	n_assert_dbg(Value.IsA<T>());
	return static_cast<T*>(&Value);
#endif
}
//---------------------------------------------------------------------

template<typename T, typename TSrc>
static inline void ConvertAndWrite(CD3D11ConstantBuffer* pCB, U32 Offset, const TSrc* pValue, UPTR Count, UPTR DestSize)
{
	Count = std::min(Count, DestSize / sizeof(T));
	const auto* pEnd = pValue + Count;
	while (pValue < pEnd)
	{
		const T Value = static_cast<T>(*pValue);
		pCB->WriteData(Offset, &Value, sizeof(T));
		++pValue;
		Offset += sizeof(T); // In D3D11 offset is in bytes // FIXME: hide in CB, return from pCB->WriteData?
	}
}
//---------------------------------------------------------------------

U32 CUSMConstantInfo::GetMemberCount() const
{
	return Struct ? Struct->Members.size() : 0;
}
//---------------------------------------------------------------------

void CUSMConstantInfo::SetRawValue(CConstantBuffer& CB, U32 Offset, const void* pValue, UPTR Size) const
{
	if (!pValue || !Size) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
		pCB->WriteData(Offset, pValue, std::min(Size, ElementCount * ElementStride)); //!!!need byte size from meta, it has no last element padding!
}
//---------------------------------------------------------------------

void CUSMConstantInfo::SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		const auto SizeInBytes = ElementCount * ElementStride; //!!!need byte size from meta, it has no last element padding!

		switch (Type)
		{
			case USMConst_Float: pCB->WriteData(Offset, pValue, std::min(Count * sizeof(float), SizeInBytes)); break;
			case USMConst_Int:
			case USMConst_Bool:  ConvertAndWrite<I32>(pCB, Offset, pValue, Count, SizeInBytes); break;
			default: ::Sys::Error("CUSMConstantInfo::SetFloats() > typed value writing allowed only for float, int & bool constants"); return;
		}
	}
}
//---------------------------------------------------------------------

void CUSMConstantInfo::SetInts(CConstantBuffer& CB, U32 Offset, const I32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		const auto SizeInBytes = ElementCount * ElementStride;

		switch (Type)
		{
			case USMConst_Float: ConvertAndWrite<float>(pCB, Offset, pValue, Count, SizeInBytes); break;
			case USMConst_Int:
			case USMConst_Bool:  pCB->WriteData(Offset, pValue, std::min(Count * sizeof(I32), SizeInBytes)); break;
			default: ::Sys::Error("CUSMConstantInfo::SetInts() > typed value writing allowed only for float, int & bool constants"); return;
		}
	}
}
//---------------------------------------------------------------------

void CUSMConstantInfo::SetUInts(CConstantBuffer& CB, U32 Offset, const U32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		const auto SizeInBytes = ElementCount * ElementStride;

		switch (Type)
		{
			case USMConst_Float: ConvertAndWrite<float>(pCB, Offset, pValue, Count, SizeInBytes); break;
			case USMConst_Int:
			case USMConst_Bool:  pCB->WriteData(Offset, pValue, std::min(Count * sizeof(U32), SizeInBytes)); break;
			default: ::Sys::Error("CUSMConstantInfo::SetUInts() > typed value writing allowed only for float, int & bool constants"); return;
		}
	}
}
//---------------------------------------------------------------------

void CUSMConstantInfo::SetBools(CConstantBuffer& CB, U32 Offset, const bool* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
	{
		const auto SizeInBytes = ElementCount * ElementStride;

		switch (Type)
		{
			case USMConst_Float: ConvertAndWrite<float>(pCB, Offset, pValue, Count, SizeInBytes); break;
			case USMConst_Int:
			case USMConst_Bool:  ConvertAndWrite<I32>(pCB, Offset, pValue, Count, SizeInBytes); break;
			default: ::Sys::Error("CUSMConstantInfo::SetBools() > typed value writing allowed only for float, int & bool constants"); return;
		}
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
