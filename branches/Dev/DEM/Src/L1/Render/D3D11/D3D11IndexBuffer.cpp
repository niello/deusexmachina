#include "D3D11IndexBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11IndexBuffer, 'IB11', Render::CIndexBuffer);

//!!!???assert destroyed?!
bool CD3D11IndexBuffer::Create(EIndexType Type, ID3D11Buffer* pIB)
{
	if (!pIB) FAIL;

	D3D11_BUFFER_DESC D3DDesc;
	pIB->GetDesc(&D3DDesc);

	if (!(D3DDesc.BindFlags & D3D11_BIND_INDEX_BUFFER)) FAIL;

	Access.ResetTo(Access_GPU_Read); //???staging to?
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) Access.Set(Access_CPU_Read);
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) Access.Set(Access_CPU_Write);
	if (D3DDesc.Usage == D3D11_USAGE_DEFAULT || D3DDesc.Usage == D3D11_USAGE_STAGING) Access.Set(Access_GPU_Write); //???staging to?

	IndexType = Type;
	IndexCount = D3DDesc.ByteWidth / (DWORD)Type;
	pBuffer = pIB;
	D3DUsage = D3DDesc.Usage;

	OK;
}
//---------------------------------------------------------------------

void CD3D11IndexBuffer::InternalDestroy()
{
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

}
