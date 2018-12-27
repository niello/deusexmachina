#include "D3D9IndexBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9IndexBuffer, 'IB09', Render::CIndexBuffer);

//!!!???assert destroyed?!
bool CD3D9IndexBuffer::Create(EIndexType Type, IDirect3DIndexBuffer9* pIB)
{
	if (!pIB) FAIL;

	D3DINDEXBUFFER_DESC D3DDesc;
	if (FAILED(pIB->GetDesc(&D3DDesc))) FAIL;

	Access.ClearAll();
	if (D3DDesc.Pool == D3DPOOL_SYSTEMMEM || D3DDesc.Pool == D3DPOOL_SCRATCH)
	{
		Access.Set(Access_CPU_Read | Access_CPU_Write);
	}
	else
	{
		Access.Set(Access_GPU_Read);
		if (D3DDesc.Usage & D3DUSAGE_DYNAMIC) Access.Set(Access_CPU_Write);
		else Access.Set(Access_GPU_Write);
	}

	IndexType = Type;
	IndexCount = D3DDesc.Size / (UPTR)Type;
	pBuffer = pIB;
	Usage = D3DDesc.Usage;
	//LockCount = 0;

	OK;
}
//---------------------------------------------------------------------

void CD3D9IndexBuffer::InternalDestroy()
{
	//n_assert(!LockCount);
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

}
