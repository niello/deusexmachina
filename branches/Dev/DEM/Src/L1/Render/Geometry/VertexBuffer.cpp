#include "IndexBuffer.h"

#include <Render/Renderer.h>
#include <d3d9.h>

namespace Render
{

bool CVertexBuffer::Create(PVertexLayout VertexLayout, DWORD VertexCount, EUsage BufferUsage, ECPUAccess BufferAccess)
{
	n_assert(VertexCount && VertexLayout.isvalid());

	Layout = VertexLayout;
	Count = VertexCount;
	Usage = BufferUsage;
	Access = BufferAccess;

	DWORD Size = Count * Layout->GetVertexSize();
	n_assert(Size);

	D3DPOOL D3DPool;
	DWORD D3DUsage;
	switch (Usage)
	{
		case UsageImmutable:
			n_assert(Access == AccessNone);
			D3DPool = D3DPOOL_MANAGED;
			D3DUsage = D3DUSAGE_WRITEONLY;
			break;
		case UsageDynamic:
			n_assert(Access == AccessWrite);
			D3DPool = D3DPOOL_DEFAULT;
			D3DUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
			break;
		case UsageCPU:
			D3DPool = D3DPOOL_SYSTEMMEM;
			D3DUsage = D3DUSAGE_DYNAMIC;
			break;
		default: n_error("Invalid IndexBuffer usage!");
	}

	HRESULT hr = RenderSrv->GetD3DDevice()->CreateVertexBuffer(Size, D3DUsage, 0, D3DPool, &pBuffer, NULL);
	if (FAILED(hr)) FAIL;

	if (D3DPool == D3DPOOL_DEFAULT)
	{
		//!!!subscribe lost & reset!
	}

	OK;
}
//---------------------------------------------------------------------

void CVertexBuffer::Destroy()
{
	n_assert(!LockCount);
	//!!!unsubscribe lost & reset!
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

void* CVertexBuffer::Map(EMapType MapType)
{
	n_assert(pBuffer);

	DWORD LockFlags = 0;
	switch (MapType)
	{
		case MapRead:
			//n_assert((Usage == UsageDynamic || Usage == UsageCPU) && (Access == AccessRead));
			break;
		case MapWrite:
			n_assert((Usage == UsageDynamic || Usage == UsageCPU) && (Access == AccessWrite));
			break;
		case MapReadWrite:
			n_assert((Usage == UsageDynamic || Usage == UsageCPU) && (Access == AccessReadWrite));
			break;
		case MapWriteDiscard:
			n_assert((Usage == UsageDynamic) && (Access == AccessWrite));
			LockFlags |= D3DLOCK_DISCARD;
			break;
		case MapWriteNoOverwrite:
			n_assert((Usage == UsageDynamic) && (Access == AccessWrite));
			LockFlags |= D3DLOCK_NOOVERWRITE;
			break;
	}

	void* pData = NULL;
	n_assert(SUCCEEDED(pBuffer->Lock(0, 0, &pData, LockFlags)));
	++LockCount;
	return pData;
}
//---------------------------------------------------------------------

void CVertexBuffer::Unmap()
{
	n_assert(pBuffer && LockCount);
	n_assert(SUCCEEDED(pBuffer->Unlock()));
	--LockCount;
}
//---------------------------------------------------------------------

}