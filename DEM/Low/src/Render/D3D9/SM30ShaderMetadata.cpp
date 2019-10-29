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

CSM30ConstantParam::CSM30ConstantParam(PSM30ConstantBufferParam Buffer, PSM30ConstantMeta Meta, ESM30RegisterSet RegisterSet, U32 Offset)
	: _Buffer(Buffer)
	, _Meta(Meta)
	, _RegisterSet(RegisterSet)
{
	n_assert_dbg(_Buffer && _RegisterSet != Reg_Invalid);

	_SizeInBytes = _Meta->ElementCount * _Meta->ElementRegisterCount;
	CSM30ConstantBufferParam::CRanges* pRanges = nullptr;
	switch (_RegisterSet)
	{
		case Reg_Float4: pRanges = &_Buffer->Float4; _SizeInBytes *= sizeof(float) * 4; break;
		case Reg_Int4:   pRanges = &_Buffer->Int4;   _SizeInBytes *= sizeof(int) * 4;   break;
		case Reg_Bool:   pRanges = &_Buffer->Bool;   _SizeInBytes *= sizeof(BOOL);      break;
	};

	// Offset is local and referenced value is always in the same range as the base
	_RegisterStart = Offset;

	for (const auto& Range : *pRanges)
	{
		// Ranges are sorted ascending, so that means that the buffer has no required range
		if (Range.first > _Meta->RegisterStart) break;

		if (Range.first + Range.second > _Meta->RegisterStart)
		{
			// Found range
			n_assert_dbg(Range.first + Range.second >= _Meta->RegisterStart + _Meta->ElementRegisterCount * _Meta->ElementCount);
			_RegisterStart += _Meta->RegisterStart - Range.first;
			return;
		}

		_RegisterStart += Range.second;
	}

	::Sys::Error("CSM30ConstantParam() > provided buffer doesn't contain the constant");
}
//---------------------------------------------------------------------

IConstantBufferParam& CSM30ConstantParam::GetConstantBuffer() const
{
	return *_Buffer;
}
//---------------------------------------------------------------------

void CSM30ConstantParam::SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const
{
	n_assert_dbg(_RegisterSet != Reg_Invalid);

	if (auto pCB = Cast<CD3D9ConstantBuffer>(CB))
		pCB->WriteData(_RegisterSet, _RegisterStart, pValue, std::min(Size, _SizeInBytes));
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
