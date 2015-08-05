#include "D3D11ConstantBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11ConstantBuffer, 'CB11', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D11ConstantBuffer::Create(ID3D11Buffer* pCB, ID3D11DeviceContext* pD3DDeviceCtx, bool StoreRAMCopy)
{
	if (!pCB || !pD3DDeviceCtx) FAIL;

	D3D11_BUFFER_DESC D3DDesc;
	pCB->GetDesc(&D3DDesc);

	//???allow texture buffers?
	if (!(D3DDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)) FAIL;

	Flags.ClearAll();

	if (StoreRAMCopy)
	{
		pMapped = (char*)n_malloc_aligned(D3DDesc.ByteWidth, 16);
		memset(pMapped, 0, D3DDesc.ByteWidth);
		Flags.Set(UsesRAMCopy);
	}

	pBuffer = pCB;
	pD3DCtx = pD3DDeviceCtx;
	pD3DCtx->AddRef();
	D3DUsage = D3DDesc.Usage;
	SizeInBytes = D3DDesc.ByteWidth;

	OK;
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::InternalDestroy()
{
	SAFE_RELEASE(pBuffer);
	SAFE_RELEASE(pD3DCtx);
	if (Flags.Is(UsesRAMCopy))
	{
		SAFE_FREE_ALIGNED(pMapped);
	}
	else n_assert(!pMapped);
}
//---------------------------------------------------------------------

bool CD3D11ConstantBuffer::BeginChanges()
{
	if (!pBuffer || D3DUsage == D3D11_USAGE_IMMUTABLE) FAIL;

	if (Flags.IsNot(UsesRAMCopy))
	{
		if (D3DUsage == D3D11_USAGE_DYNAMIC)
		{
			D3D11_MAPPED_SUBRESOURCE MappedSubRsrc;
			if (FAILED(pD3DCtx->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubRsrc))) FAIL;
			pMapped = (char*)MappedSubRsrc.pData;
		}
		else FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

bool CD3D11ConstantBuffer::CommitChanges()
{
	if (Flags.Is(UsesRAMCopy))
	{
		if (Flags.Is(RAMCopyDirty))
		{
			if (D3DUsage == D3D11_USAGE_DYNAMIC)
			{
				D3D11_MAPPED_SUBRESOURCE MappedSubRsrc;
				if (FAILED(pD3DCtx->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubRsrc))) FAIL;
				memcpy(MappedSubRsrc.pData, pMapped, SizeInBytes);
				pD3DCtx->Unmap(pBuffer, 0);
			}
			else if (D3DUsage == D3D11_USAGE_DEFAULT)
			{
				pD3DCtx->UpdateSubresource(pBuffer, 0, NULL, pMapped, 0, 0);
			}
			else FAIL;

			Flags.Clear(RAMCopyDirty);
		}
	}
	else if (D3DUsage == D3D11_USAGE_DYNAMIC)
	{
		if (pMapped)
		{
			n_assert_dbg(Flags.Is(RAMCopyDirty)); // Else all the buffer contents are discarded
			pD3DCtx->Unmap(pBuffer, 0);
			pMapped = NULL;
			Flags.Clear(RAMCopyDirty);
		}
	}

	OK;
}
//---------------------------------------------------------------------

// Updates whole buffer contents, especially useful for VRAM-only D3D11_USAGE_DEFAULT buffers
bool CD3D11ConstantBuffer::WriteCommitToVRAM(const void* pData)
{
	if (!pData) FAIL;

	if (D3DUsage == D3D11_USAGE_DEFAULT)
	{
		pD3DCtx->UpdateSubresource(pBuffer, 0, NULL, pData, 0, 0);
	}
	else if (D3DUsage == D3D11_USAGE_DYNAMIC)
	{
		void* pDest = Flags.Is(UsesRAMCopy) ? NULL : pMapped;
		if (pDest) pMapped = NULL;
		else
		{
			D3D11_MAPPED_SUBRESOURCE MappedSubRsrc;
			if (FAILED(pD3DCtx->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubRsrc))) FAIL;
			pDest = MappedSubRsrc.pData;
		}
		memcpy(pDest, pData, SizeInBytes);
		pD3DCtx->Unmap(pBuffer, 0);
	}
	else FAIL;

	if (Flags.Is(UsesRAMCopy))
	{
		memcpy(pMapped, pData, SizeInBytes);
		Flags.Clear(RAMCopyDirty);
	}

	OK;
}
//---------------------------------------------------------------------

}
