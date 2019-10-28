#include "SM30ShaderMetadata.h"
#include <Render/D3D9/D3D9GPUDriver.h>
#include <Render/D3D9/D3D9ConstantBuffer.h>

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

}
