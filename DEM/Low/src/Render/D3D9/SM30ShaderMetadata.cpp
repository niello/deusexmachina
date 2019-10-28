#include "SM30ShaderMetadata.h"
#include <Render/D3D9/D3D9GPUDriver.h>
#include <Render/D3D9/D3D9ConstantBuffer.h>
#include <Render/D3D9/D3D9Texture.h>
#include <Render/D3D9/D3D9Sampler.h>

namespace Render
{
__ImplementClassNoFactory(CSM30ConstantBufferParam, IConstantBufferParam);

bool CSM30ConstantBufferParam::Apply(CGPUDriver& GPU, CConstantBuffer* pValue) const
{
	auto pGPU = GPU.As<CD3D9GPUDriver>();
	if (!pGPU) FAIL;

	CD3D9ConstantBuffer* pCB;
	if (pValue)
	{
		pCB = pValue->As<CD3D9ConstantBuffer>();
		if (!pCB) FAIL;
	}
	else pCB = nullptr;

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
	const auto* pCB = Value.As<CD3D9ConstantBuffer>();
	return pCB && this == pCB->GetMetadata();
}
//---------------------------------------------------------------------

bool CSM30ResourceParam::Apply(CGPUDriver& GPU, CTexture* pValue) const
{
	auto pGPU = GPU.As<CD3D9GPUDriver>();
	if (!pGPU) FAIL;

	CD3D9Texture* pTex;
	if (pValue)
	{
		pTex = pValue->As<CD3D9Texture>();
		if (!pTex) FAIL;
	}
	else pTex = nullptr;

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
	auto pGPU = GPU.As<CD3D9GPUDriver>();
	if (!pGPU) FAIL;

	CD3D9Sampler* pSampler;
	if (pValue)
	{
		pSampler = pValue->As<CD3D9Sampler>();
		if (!pSampler) FAIL;
	}
	else pSampler = nullptr;

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
