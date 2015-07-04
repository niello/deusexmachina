#include "D3D9VertexBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9VertexBuffer, 'VB09', Render::CVertexBuffer);

bool CD3D9VertexBuffer::Create(CVertexLayout& Layout, IDirect3DVertexBuffer9* pVB)
{
	if (!pVB) FAIL;

	DWORD VertexSize = Layout.GetVertexSize();
	if (!VertexSize) FAIL;

	D3DVERTEXBUFFER_DESC D3DDesc;
	if (FAILED(pVB->GetDesc(&D3DDesc))) FAIL;

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

	VertexLayout = &Layout;
	VertexCount = D3DDesc.Size / VertexSize;
	pBuffer = pVB;
	LockCount = 0;

	OK;
}
//---------------------------------------------------------------------

void CD3D9VertexBuffer::InternalDestroy()
{
	n_assert(!LockCount); //???who must track and change this value?
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

}