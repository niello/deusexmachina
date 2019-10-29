#include "USMShaderMetadata.h"
#include <Render/D3D11/D3D11GPUDriver.h>
#include <Render/D3D11/D3D11ConstantBuffer.h>
#include <Render/D3D11/D3D11Texture.h>
#include <Render/D3D11/D3D11Sampler.h>

namespace Render
{
__ImplementClassNoFactory(CUSMConstantBufferParam, IConstantBufferParam);

IConstantBufferParam& CUSMConstantParam::GetConstantBuffer() const
{
	return *_Buffer;
}
//---------------------------------------------------------------------

void CUSMConstantParam::SetRawValue(CConstantBuffer& CB, const void* pValue, UPTR Size) const
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

bool CUSMConstantBufferParam::Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const
{
	auto pGPU = GPU.As<CD3D11GPUDriver>();
	if (!pGPU) FAIL;

	CD3D11ConstantBuffer* pCB;
	if (pValue)
	{
		pCB = pValue->As<CD3D11ConstantBuffer>();
		if (!pCB) FAIL;
	}
	else pCB = nullptr;

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
	const auto* pCB = Value.As<CD3D11ConstantBuffer>();
	return pCB && Type == pCB->GetType() && Size <= pCB->GetSizeInBytes();
}
//---------------------------------------------------------------------

bool CUSMResourceParam::Apply(CGPUDriver& GPU, CTexture* pValue) const
{
	auto pGPU = GPU.As<CD3D11GPUDriver>();
	if (!pGPU) FAIL;

	CD3D11Texture* pTex;
	if (pValue)
	{
		pTex = pValue->As<CD3D11Texture>();
		if (!pTex) FAIL;
	}
	else pTex = nullptr;

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
	auto pGPU = GPU.As<CD3D11GPUDriver>();
	if (!pGPU) FAIL;

	CD3D11Sampler* pSampler;
	if (pValue)
	{
		pSampler = pValue->As<CD3D11Sampler>();
		if (!pSampler) FAIL;
	}
	else pSampler = nullptr;

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
