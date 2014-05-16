#include "IndexBuffer.h"

#include <Render/RenderServer.h>
#include <Events/EventServer.h>

namespace Render
{

bool CIndexBuffer::Create(EType IndexType, DWORD IndexCount, EUsage BufferUsage, ECPUAccess BufferAccess)
{
	n_assert(IndexCount);

	Count = IndexCount;
	Type = IndexType;
	Usage = BufferUsage;
	Access = BufferAccess;

	DWORD Size;
	D3DFORMAT D3DFormat;
	if (Type == Index16)
	{
		Size = Count * 2;
		D3DFormat = D3DFMT_INDEX16;
	}
	else
	{
		Size = Count * 4;
		D3DFormat = D3DFMT_INDEX32;
	}

	D3DPOOL D3DPool;
	DWORD D3DUsage;
	switch (Usage)
	{
		case Usage_Immutable:
			n_assert(Access == CPU_NoAccess);
			D3DPool = D3DPOOL_MANAGED;
			D3DUsage = D3DUSAGE_WRITEONLY;
			break;
		case Usage_Dynamic:
			n_assert(Access == CPU_Write);
			D3DPool = D3DPOOL_DEFAULT;
			D3DUsage = D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY;
			break;
		case Usage_CPU:
			D3DPool = D3DPOOL_SYSTEMMEM;
			D3DUsage = D3DUSAGE_DYNAMIC;
			break;
		default: Sys::Error("Invalid IndexBuffer usage!");
	}

	if (FAILED(RenderSrv->GetD3DDevice()->CreateIndexBuffer(Size, D3DUsage, D3DFormat, D3DPool, &pBuffer, NULL))) FAIL;

	if (D3DPool == D3DPOOL_DEFAULT)
		SUBSCRIBE_PEVENT(OnRenderDeviceLost, CIndexBuffer, OnDeviceLost);

	OK;
}
//---------------------------------------------------------------------

void CIndexBuffer::Destroy()
{
	n_assert(!LockCount);

	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);

	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

void* CIndexBuffer::Map(EMapType MapType)
{
	n_assert(pBuffer);

	DWORD LockFlags = 0;
	switch (MapType)
	{
		case Map_Setup:
			LockFlags |= D3DLOCK_NOSYSLOCK;
			break;
		case Map_Read:
			n_assert((Usage == Usage_Dynamic || Usage == Usage_CPU) && (Access == CPU_Read));
			break;
		case Map_Write:
			n_assert((Usage == Usage_Dynamic || Usage == Usage_CPU) && (Access == CPU_Write));
			break;
		case Map_ReadWrite:
			n_assert((Usage == Usage_Dynamic || Usage == Usage_CPU) && (Access == CPU_ReadWrite));
			break;
		case Map_WriteDiscard:
			n_assert((Usage == Usage_Dynamic) && (Access == CPU_Write));
			LockFlags |= D3DLOCK_DISCARD;
			break;
		case Map_WriteNoOverwrite:
			n_assert((Usage == Usage_Dynamic) && (Access == CPU_Write));
			LockFlags |= D3DLOCK_NOOVERWRITE;
			break;
	}

	void* pData = NULL;
	n_assert(SUCCEEDED(pBuffer->Lock(0, 0, &pData, LockFlags)));
	++LockCount;
	return pData;
}
//---------------------------------------------------------------------

void CIndexBuffer::Unmap()
{
	n_assert(pBuffer && LockCount);
	n_assert(SUCCEEDED(pBuffer->Unlock()));
	--LockCount;
}
//---------------------------------------------------------------------

bool CIndexBuffer::OnDeviceLost(const Events::CEventBase& Ev)
{
	Destroy();
	OK;
}
//---------------------------------------------------------------------

}