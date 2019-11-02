#include "SM30ShaderMetadata.h"
#include <Render/D3D9/D3D9GPUDriver.h>
#include <Render/D3D9/D3D9ConstantBuffer.h>
#include <Render/D3D9/D3D9Texture.h>
#include <Render/D3D9/D3D9Sampler.h>

#undef min
#undef max

// NB: HLSL bool is BOOL, which is a 32-bit signed int

namespace Render
{
__ImplementClassNoFactory(CSM30ConstantBufferParam, IConstantBufferParam);

template<typename T>
static inline T* Cast(Core::CRTTIBaseClass& Value)
{
#if DEM_SHADER_META_DYNAMIC_TYPE_VALIDATION
	return Value.As<T>();
#else
	n_assert_dbg(Value.IsA<T>());
	return static_cast<T*>(&Value);
#endif
}
//---------------------------------------------------------------------

template<typename T, typename TSrc>
static inline void ConvertAndWrite(CD3D9ConstantBuffer* pCB, ESM30RegisterSet RegSet, U32 Offset, const TSrc* pValue, UPTR Count, UPTR DestSize)
{
	// FIXME: all components are known to be 32-bit, instead of DestSize can cache DestComponentCount and static_assert sizeof(T)
	Count = std::min(Count, DestSize / sizeof(T));
	const auto* pEnd = pValue + Count;
	while (pValue < pEnd)
	{
		const T Value = static_cast<T>(*pValue);
		pCB->WriteData(RegSet, Offset, &Value, sizeof(T));
		++pValue;
		Offset += sizeof(T); // In D3D11 offset is in bytes // FIXME: hide in CB, return from pCB->WriteData?
	}
}
//---------------------------------------------------------------------

CSM30ConstantInfo::CSM30ConstantInfo(size_t ConstantBufferIndex, const CSM30ConstantMeta& Meta, ESM30RegisterSet RegisterSet)
	: CShaderConstantInfo(ConstantBufferIndex, Meta)
	, _Struct(Meta.Struct)
	, _RegisterSet(RegisterSet)
{
}
//---------------------------------------------------------------------

U32 CSM30ConstantInfo::GetMemberCount() const
{
	return _Struct ? _Struct->Members.size() : 0;
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetRawValue(CConstantBuffer& CB, U32 Offset, const void* pValue, UPTR Size) const
{
	if (!pValue || !Size) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
		pCB->WriteData(_RegisterSet, Offset, pValue, std::min(Size, _Meta.ElementCount * _Meta.ElementStride)); //!!!need byte size from meta, it has no last element padding!
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		const auto SizeInBytes = _Meta.ElementCount * _Meta.ElementStride; //!!!need byte size from meta, it has no last element padding!

		switch (_RegisterSet)
		{
			case Reg_Float4: pCB->WriteData(Reg_Float4, Offset, pValue, std::min(Count * sizeof(float), SizeInBytes)); break;
			case Reg_Int4:   ConvertAndWrite<I32>(pCB, _RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Bool:   ConvertAndWrite<BOOL>(pCB, _RegisterSet, Offset, pValue, Count, SizeInBytes); break;
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetInts(CConstantBuffer& CB, U32 Offset, const I32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		const auto SizeInBytes = _Meta.ElementCount * _Meta.ElementStride; //!!!need byte size from meta, it has no last element padding!

		switch (_RegisterSet)
		{
			case Reg_Float4: ConvertAndWrite<float>(pCB, _RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Int4:
			case Reg_Bool:   pCB->WriteData(Reg_Int4, Offset, pValue, std::min(Count * sizeof(I32), SizeInBytes)); break;
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetUInts(CConstantBuffer& CB, U32 Offset, const U32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		const auto SizeInBytes = _Meta.ElementCount * _Meta.ElementStride; //!!!need byte size from meta, it has no last element padding!

		// NB: no need to convert U32 to I32, write as is
		switch (_RegisterSet)
		{
			case Reg_Float4: ConvertAndWrite<float>(pCB, _RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Int4:
			case Reg_Bool:   pCB->WriteData(Reg_Int4, Offset, pValue, std::min(Count * sizeof(U32), SizeInBytes)); break;
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetBools(CConstantBuffer& CB, U32 Offset, const bool* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		const auto SizeInBytes = _Meta.ElementCount * _Meta.ElementStride; //!!!need byte size from meta, it has no last element padding!

		switch (_RegisterSet)
		{
			case Reg_Float4: ConvertAndWrite<float>(pCB, _RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Int4:   ConvertAndWrite<I32>(pCB, _RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Bool:   ConvertAndWrite<BOOL>(pCB, _RegisterSet, Offset, pValue, Count, SizeInBytes); break;
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
