#include "D3D9VertexBuffer.h"

#include <Core/Factory.h>
#include "DEMD3D9.h"
#if DEM_RENDER_DEBUG
#include <D3Dcommon.h> // WKPDID_D3DDebugObjectName
#endif

namespace Render
{
FACTORY_CLASS_IMPL(Render::CD3D9VertexBuffer, 'VB09', Render::CVertexBuffer);

//!!!???assert destroyed?!
bool CD3D9VertexBuffer::Create(CVertexLayout& Layout, IDirect3DVertexBuffer9* pVB)
{
	if (!pVB) FAIL;

	UPTR VertexSize = Layout.GetVertexSizeInBytes();
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
	Usage = D3DDesc.Usage;
	//LockCount = 0;

	OK;
}
//---------------------------------------------------------------------

void CD3D9VertexBuffer::InternalDestroy()
{
	//n_assert(!LockCount); //???who must track and change this value?
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

void CD3D9VertexBuffer::SetDebugName(std::string_view Name)
{
#if DEM_RENDER_DEBUG
	if (pBuffer) pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, Name.data(), Name.size(), 0);
#endif
}
//---------------------------------------------------------------------

}
