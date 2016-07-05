#include "D3D11ConstantBuffer.h"

#include <Render/D3D11/USMShaderMetadata.h> // For EUSMBufferType only
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11ConstantBuffer, 'CB11', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D11ConstantBuffer::Create(ID3D11Buffer* pCB, ID3D11ShaderResourceView* pSRV)
{
	if (!pCB) FAIL;

	D3D11_BUFFER_DESC D3DDesc;
	pCB->GetDesc(&D3DDesc);

	if (!(D3DDesc.Usage & D3D11_USAGE_STAGING) &&
		!((D3DDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) || (pSRV && (D3DDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE))))
	{
		FAIL;
	}

	Flags.ClearAll();

	pBuffer = pCB;
	pSRView = pSRV;
	D3DUsage = D3DDesc.Usage;
	SizeInBytes = D3DDesc.ByteWidth;

	if (D3DDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER) Type = USMBuffer_Constant;
	else if (D3DDesc.MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) Type = USMBuffer_Structured;
	else Type = USMBuffer_Texture;

	OK;
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::InternalDestroy()
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

void CD3D11ConstantBuffer::ResetRAMCopy(const void* pVRAMData)
{
	if (Flags.Is(CB11_UsesRAMCopy))
	{
		if (pVRAMData) memcpy(pMapped, pVRAMData, SizeInBytes);
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
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::OnCommit()
{
	if (Flags.IsNot(CB11_UsesRAMCopy)) pMapped = NULL;
	Flags.Clear(CB11_Dirty);
}
//---------------------------------------------------------------------

}
