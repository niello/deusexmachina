#include "D3D9IndexBuffer.h"

#include <Core/Factory.h>
#include "DEMD3D9.h"
#include <D3Dcommon.h> // WKPDID_D3DDebugObjectName

namespace Render
{
FACTORY_CLASS_IMPL(Render::CD3D9IndexBuffer, 'IB09', Render::CIndexBuffer);

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

void CD3D9IndexBuffer::SetDebugName(std::string_view Name)
{
	if (pBuffer) pBuffer->SetPrivateData(WKPDID_D3DDebugObjectName, Name.data(), Name.size(), 0);
}
//---------------------------------------------------------------------

}
