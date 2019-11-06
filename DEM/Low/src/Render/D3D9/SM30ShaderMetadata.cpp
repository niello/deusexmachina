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

PShaderConstantInfo CSM30ConstantInfo::Clone() const
{
	PSM30ConstantInfo Info = n_new(CSM30ConstantInfo);
	Info->CShaderConstantInfo_CopyFields(*this);
	Info->RegisterSet = RegisterSet;
	return Info;
}
//---------------------------------------------------------------------

// FIXME: to constructor? or even to shader metadata file?
// Packing is by whole registers, and only BOOL registers are scalar, others are 4-component
void CSM30ConstantInfo::CalculateCachedValues()
{
	ComponentSize = 4; // All register components are 32-bit

	if (Struct)
	{
		VectorStride = 0;
		ElementSize = ElementStride;
	}
	else
	{
		const auto MajorDim = IsColumnMajor() ? Columns : Rows;
		const auto MinorDim = IsColumnMajor() ? Rows : Columns;
		if (RegisterSet == Reg_Bool)
		{
			// TODO: could check how bool vectors and matrices work on sm3.0,
			// but no one probably will ever use that
			VectorStride = MinorDim * ComponentSize;
			ElementSize = MajorDim * VectorStride;
		}
		else
		{
			VectorStride = (MajorDim > 1 ? 4 : MinorDim) * ComponentSize;
			ElementSize = (MajorDim - 1) * VectorStride + MinorDim * ComponentSize;
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetRawValue(CConstantBuffer& CB, U32 Offset, const void* pValue, UPTR Size) const
{
	if (!pValue || !Size) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
		pCB->WriteData(RegisterSet, Offset, pValue, std::min(Size, ElementCount * ElementStride)); //!!!need byte size from meta, it has no last element padding!
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetFloats(CConstantBuffer& CB, U32 Offset, const float* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		const auto SizeInBytes = ElementCount * ElementStride; //!!!need byte size from meta, it has no last element padding!

		switch (RegisterSet)
		{
			case Reg_Float4: pCB->WriteData(Reg_Float4, Offset, pValue, std::min(Count * sizeof(float), SizeInBytes)); break;
			case Reg_Int4:   ConvertAndWrite<I32>(pCB, RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Bool:   ConvertAndWrite<BOOL>(pCB, RegisterSet, Offset, pValue, Count, SizeInBytes); break;
		}
	}
}
//---------------------------------------------------------------------

void CSM30ConstantInfo::SetInts(CConstantBuffer& CB, U32 Offset, const I32* pValue, UPTR Count) const
{
	if (!pValue || !Count) return;

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
	{
		const auto SizeInBytes = ElementCount * ElementStride; //!!!need byte size from meta, it has no last element padding!

		switch (RegisterSet)
		{
			case Reg_Float4: ConvertAndWrite<float>(pCB, RegisterSet, Offset, pValue, Count, SizeInBytes); break;
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
		const auto SizeInBytes = ElementCount * ElementStride; //!!!need byte size from meta, it has no last element padding!

		// NB: no need to convert U32 to I32, write as is
		switch (RegisterSet)
		{
			case Reg_Float4: ConvertAndWrite<float>(pCB, RegisterSet, Offset, pValue, Count, SizeInBytes); break;
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
		const auto SizeInBytes = ElementCount * ElementStride; //!!!need byte size from meta, it has no last element padding!

		switch (RegisterSet)
		{
			case Reg_Float4: ConvertAndWrite<float>(pCB, RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Int4:   ConvertAndWrite<I32>(pCB, RegisterSet, Offset, pValue, Count, SizeInBytes); break;
			case Reg_Bool:   ConvertAndWrite<BOOL>(pCB, RegisterSet, Offset, pValue, Count, SizeInBytes); break;
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

	if (ShaderTypeMask & (1 << ShaderType_Vertex))
	{
		if (!pGPU->BindConstantBuffer(ShaderType_Vertex, SlotIndex, pCB)) FAIL;
	}

	if (ShaderTypeMask & (1 << ShaderType_Pixel))
	{
		if (!pGPU->BindConstantBuffer(ShaderType_Pixel, SlotIndex, pCB)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CSM30ConstantBufferParam::Unapply(CGPUDriver& GPU, CConstantBuffer* pValue) const
{
	if (!pValue) return;

	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) return;

	auto pCB = Cast<CD3D9ConstantBuffer>(*pValue);
	if (!pCB) return;

	if (ShaderTypeMask & (1 << ShaderType_Vertex))
	{
		pGPU->UnbindConstantBuffer(ShaderType_Vertex, SlotIndex, *pCB);
	}

	if (ShaderTypeMask & (1 << ShaderType_Pixel))
	{
		pGPU->UnbindConstantBuffer(ShaderType_Pixel, SlotIndex, *pCB);
	}
}
//---------------------------------------------------------------------

bool CSM30ConstantBufferParam::IsBufferCompatible(CConstantBuffer& Value) const
{
	const auto* pCB = Cast<CD3D9ConstantBuffer>(Value);
	return pCB && this == pCB->GetMetadata();
}
//---------------------------------------------------------------------

CSM30ResourceParam::CSM30ResourceParam(CStrID Name, U8 ShaderTypeMask, U32 Register)
	: _Name(Name)
	, _Register(Register)
	, _ShaderTypeMask(ShaderTypeMask)
{
}
//---------------------------------------------------------------------

bool CSM30ResourceParam::Apply(CGPUDriver& GPU, CTexture* pValue) const
{
	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pTex = pValue ? Cast<CD3D9Texture>(*pValue) : nullptr;
	if (pValue && !pTex) FAIL;

	if (_ShaderTypeMask & (1 << ShaderType_Vertex))
	{
		if (!pGPU->BindResource(ShaderType_Vertex, _Register, pTex)) FAIL;
	}

	if (_ShaderTypeMask & (1 << ShaderType_Pixel))
	{
		if (!pGPU->BindResource(ShaderType_Pixel, _Register, pTex)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CSM30ResourceParam::Unapply(CGPUDriver& GPU, CTexture* pValue) const
{
	if (!pValue) return;

	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) return;

	auto pTex = Cast<CD3D9Texture>(*pValue);
	if (!pTex) return;

	if (_ShaderTypeMask & (1 << ShaderType_Vertex))
	{
		pGPU->UnbindResource(ShaderType_Vertex, _Register, *pTex);
	}

	if (_ShaderTypeMask & (1 << ShaderType_Pixel))
	{
		pGPU->UnbindResource(ShaderType_Pixel, _Register, *pTex);
	}
}
//---------------------------------------------------------------------

CSM30SamplerParam::CSM30SamplerParam(CStrID Name, U8 ShaderTypeMask, ESM30SamplerType Type, U32 RegisterStart, U32 RegisterCount)
	: _Name(Name)
	, _Type(Type)
	, _RegisterStart(RegisterStart)
	, _RegisterCount(RegisterCount)
	, _ShaderTypeMask(ShaderTypeMask)
{
}
//---------------------------------------------------------------------

bool CSM30SamplerParam::Apply(CGPUDriver& GPU, CSampler* pValue) const
{
	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) FAIL;

	auto pSampler = pValue ? Cast<CD3D9Sampler>(*pValue) : nullptr;
	if (pValue && !pSampler) FAIL;

	if (_ShaderTypeMask & (1 << ShaderType_Vertex))
	{
		if (!pGPU->BindSampler(ShaderType_Vertex, _RegisterStart, _RegisterCount, pSampler)) FAIL;
	}

	if (_ShaderTypeMask & (1 << ShaderType_Pixel))
	{
		if (!pGPU->BindSampler(ShaderType_Pixel, _RegisterStart, _RegisterCount, pSampler)) FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CSM30SamplerParam::Unapply(CGPUDriver& GPU, CSampler* pValue) const
{
	if (!pValue) return;

	auto pGPU = Cast<CD3D9GPUDriver>(GPU);
	if (!pGPU) return;

	auto pSampler = Cast<CD3D9Sampler>(*pValue);
	if (!pSampler) return;

	if (_ShaderTypeMask & (1 << ShaderType_Vertex))
	{
		pGPU->UnbindSampler(ShaderType_Vertex, _RegisterStart, _RegisterCount, *pSampler);
	}

	if (_ShaderTypeMask & (1 << ShaderType_Pixel))
	{
		pGPU->UnbindSampler(ShaderType_Pixel, _RegisterStart, _RegisterCount, *pSampler);
	}
}
//---------------------------------------------------------------------

}
