//#include "D3D9IndexBuffer.h"
//
//#include <Render/RenderServer.h>
//#include <Events/EventServer.h>
//
//namespace Render
//{
//
//void CD3D9IndexBuffer::InternalDestroy()
//{
//	n_assert(!LockCount);
//	UNSUBSCRIBE_EVENT(OnRenderDeviceLost);
//	SAFE_RELEASE(pBuffer);
//}
////---------------------------------------------------------------------
//
//void* CD3D9IndexBuffer::Map(EMapType MapType)
//{
//	n_assert(pBuffer);
//
//	DWORD LockFlags = 0;
//	switch (MapType)
//	{
//		case Map_Setup:
//			LockFlags |= D3DLOCK_NOSYSLOCK;
//			break;
//		case Map_Read:
//			n_assert(Access.Is(CPU_Read));
//			break;
//		case Map_Write:
//			n_assert(Access.Is(CPU_Write));
//			break;
//		case Map_ReadWrite:
//			n_assert(Access.Is(CPU_Read | CPU_Write));
//			break;
//		case Map_WriteDiscard:
//			n_assert(Access.Is(GPU_Read | CPU_Write));
//			LockFlags |= D3DLOCK_DISCARD;
//			break;
//		case Map_WriteNoOverwrite:
//			n_assert(Access.Is(GPU_Read | CPU_Write));
//			LockFlags |= D3DLOCK_NOOVERWRITE;
//			break;
//	}
//
//	void* pData = NULL;
//	n_assert(SUCCEEDED(pBuffer->Lock(0, 0, &pData, LockFlags)));
//	++LockCount;
//	return pData;
//}
////---------------------------------------------------------------------
//
//void CD3D9IndexBuffer::Unmap()
//{
//	n_assert(pBuffer && LockCount);
//	n_assert(SUCCEEDED(pBuffer->Unlock()));
//	--LockCount;
//}
////---------------------------------------------------------------------
//
//bool CD3D9IndexBuffer::OnDeviceLost(const Events::CEventBase& Ev)
//{
//	Destroy();
//	OK;
//}
////---------------------------------------------------------------------
//
//}
