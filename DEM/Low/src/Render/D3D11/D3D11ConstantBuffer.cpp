#include "D3D11ConstantBuffer.h"
#include <Render/D3D11/USMShaderMetadata.h> // For EUSMBufferType only
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
RTTI_CLASS_IMPL(Render::CD3D11ConstantBuffer, Render::CConstantBuffer);

CD3D11ConstantBuffer::CD3D11ConstantBuffer(ID3D11Buffer* pCB, ID3D11ShaderResourceView* pSRV, bool Temporary)
{
	if (!pCB) return;

	D3D11_BUFFER_DESC D3DDesc;
	pCB->GetDesc(&D3DDesc);

	if (!(D3DDesc.Usage & D3D11_USAGE_STAGING) &&
		!((D3DDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) || (pSRV && (D3DDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE))))
	{
		return;
	}

	Flags.ClearAll();
	if (Temporary) Flags.Set(CB11_Temporary);

	pBuffer = pCB;
	pSRView = pSRV;
	D3DUsage = D3DDesc.Usage;
	SizeInBytes = D3DDesc.ByteWidth;

	if (D3DDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) Type = USMBuffer_Constant;
	else if (D3DDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) Type = USMBuffer_Structured;
	else Type = USMBuffer_Texture;
}
//---------------------------------------------------------------------

CD3D11ConstantBuffer::~CD3D11ConstantBuffer()
{
	SAFE_RELEASE(pSRView);
	SAFE_RELEASE(pBuffer);
	if (Flags.Is(CB11_UsesRAMCopy))
	{
		n_assert_dbg(!SizeInBytes || pMapped);
		SAFE_FREE_ALIGNED(pMapped);
	}
	else n_assert(!pMapped);
}
//---------------------------------------------------------------------

U8 CD3D11ConstantBuffer::GetAccessFlags() const
{
	switch (D3DUsage)
	{
		case D3D11_USAGE_DEFAULT:   return Access_GPU_Read | Access_GPU_Write;
		case D3D11_USAGE_IMMUTABLE: return Access_GPU_Read;
		case D3D11_USAGE_DYNAMIC:   return Access_GPU_Read | Access_CPU_Write;
		case D3D11_USAGE_STAGING:   return Access_CPU_Read | Access_CPU_Write;
	}

	return 0;
}
//---------------------------------------------------------------------

bool CD3D11ConstantBuffer::CreateRAMCopy()
{
	if (Flags.Is(CB11_UsesRAMCopy)) OK;
	if (!pBuffer || !SizeInBytes) FAIL;
	pMapped = (char*)n_malloc_aligned(SizeInBytes, 16);
	if (!pMapped) FAIL;
	memset(pMapped, 0, SizeInBytes);
	Flags.Set(CB11_UsesRAMCopy); // | CB11_Dirty);
	OK;
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::ResetRAMCopy(const void* pVBuffer)
{
	if (Flags.Is(CB11_UsesRAMCopy))
	{
		if (pVBuffer) memcpy(pMapped, pVBuffer, SizeInBytes);
		else memset(pMapped, 0, SizeInBytes);
		Flags.Clear(CB11_Dirty);
	}
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::DestroyRAMCopy()
{
	if (Flags.IsNot(CB11_UsesRAMCopy)) return;
	SAFE_FREE_ALIGNED(pMapped);
	Flags.Clear(CB11_UsesRAMCopy | CB11_Dirty);
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::OnBegin(void* pMappedVRAM)
{
	if (Flags.Is(CB11_UsesRAMCopy))
	{
		n_assert_dbg(!pMappedVRAM);
	}
	else
	{
		n_assert_dbg(!pMapped && pMappedVRAM);
		pMapped = (char*)pMappedVRAM;
	}

	Flags.Set(CB11_InWriteMode);
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::OnCommit()
{
	if (Flags.IsNot(CB11_UsesRAMCopy)) pMapped = nullptr;
	Flags.Clear(CB11_Dirty | CB11_InWriteMode);
}
//---------------------------------------------------------------------

}
