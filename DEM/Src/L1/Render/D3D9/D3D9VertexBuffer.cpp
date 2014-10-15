#include "D3D9VertexBuffer.h"

#include <Render/RenderServer.h>
#include <Events/EventServer.h>

namespace Render
{

bool CD3D9VertexBuffer::Create(PVertexLayout VertexLayout, DWORD VertexCount, DWORD BufferAccess)
{
	n_assert(VertexCount && VertexLayout.IsValid());

	Access.ResetTo(BufferAccess);
	Layout = VertexLayout;
	VtxCount = VertexCount;

	DWORD Size = GetSizeInBytes();
	n_assert(Size);

	D3DPOOL D3DPool;
	DWORD D3DUsage;
	if (Access.Is(GPU_Read | CPU_Write))
	{
		Access.ResetTo(GPU_Read | CPU_Write);
		D3DPool = D3DPOOL_DEFAULT;
		D3DUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
	}
	else if (Access.Is(GPU_Read))
	{
		Access.ResetTo(GPU_Read);
		D3DPool = D3DPOOL_MANAGED;
		D3DUsage = D3DUSAGE_WRITEONLY;
	}
	else if (Access.Is(CPU_Read) || Access.Is(CPU_Write))
	{
		Access.ResetTo(CPU_Read | CPU_Write); // Always supports both
		D3DPool = D3DPOOL_SYSTEMMEM;
		D3DUsage = D3DUSAGE_DYNAMIC;
	}
	else Sys::Error("!!!REWRITE D3D9 BUFFER ACCESS MAPPING!!!");

	if (FAILED(RenderSrv->GetD3DDevice()->CreateVertexBuffer(Size, D3DUsage, 0, D3DPool, &pBuffer, NULL))) FAIL;

	if (D3DPool == D3DPOOL_DEFAULT)
		SUBSCRIBE_PEVENT(OnRenderDeviceLost, CD3D9VertexBuffer, OnDeviceLost);

	OK;
}
//---------------------------------------------------------------------

void CD3D9VertexBuffer::InternalDestroy()
{
	n_assert(!LockCount);
	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

void* CD3D9VertexBuffer::Map(EMapType MapType)
{
	n_assert(pBuffer);

	DWORD LockFlags = 0;
	switch (MapType)
	{
		case Map_Setup:
			LockFlags |= D3DLOCK_NOSYSLOCK;
			break;
		case Map_Read:
			n_assert(Access.Is(CPU_Read));
			break;
		case Map_Write:
			n_assert(Access.Is(CPU_Write));
			break;
		case Map_ReadWrite:
			n_assert(Access.Is(CPU_Read | CPU_Write));
			break;
		case Map_WriteDiscard:
			n_assert(Access.Is(GPU_Read | CPU_Write));
			LockFlags |= D3DLOCK_DISCARD;
			break;
		case Map_WriteNoOverwrite:
			n_assert(Access.Is(GPU_Read | CPU_Write));
			LockFlags |= D3DLOCK_NOOVERWRITE;
			break;
	}

	void* pData = NULL;
	n_assert(SUCCEEDED(pBuffer->Lock(0, 0, &pData, LockFlags)));
	++LockCount;
	return pData;
}
//---------------------------------------------------------------------

void CD3D9VertexBuffer::Unmap()
{
	n_assert(pBuffer && LockCount);
	n_assert(SUCCEEDED(pBuffer->Unlock()));
	--LockCount;
}
//---------------------------------------------------------------------

bool CD3D9VertexBuffer::OnDeviceLost(const Events::CEventBase& Ev)
{
	Destroy();
	OK;
}
//---------------------------------------------------------------------

}