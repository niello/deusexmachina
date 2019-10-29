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

IConstantBufferParam& CUSMConstantParam::GetConstantBuffer() const
{
	return *_Buffer;
}
//---------------------------------------------------------------------

void CUSMConstantParam::SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const
{
	if (auto pCB = Cast<CD3D11ConstantBuffer>(CB))
		pCB->WriteData(_OffsetInBuffer, pValue, std::min(Size, _Meta->ElementCount * _Meta->ElementSize));
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
	auto pCB = Cast<CD3D11ConstantBuffer>(Value);
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
