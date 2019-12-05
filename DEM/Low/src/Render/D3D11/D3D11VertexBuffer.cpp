#include "D3D11VertexBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
FACTORY_CLASS_IMPL(Render::CD3D11VertexBuffer, 'VB11', Render::CVertexBuffer);

//!!!???assert destroyed?!
bool CD3D11VertexBuffer::Create(CVertexLayout& Layout, ID3D11Buffer* pVB)
{
	if (!pVB) FAIL;

	UPTR VertexSize = Layout.GetVertexSizeInBytes();
	if (!VertexSize) FAIL;

	D3D11_BUFFER_DESC D3DDesc;
	pVB->GetDesc(&D3DDesc);

	if (!(D3DDesc.BindFlags & D3D11_BIND_VERTEX_BUFFER)) FAIL;

	Access.ResetTo(Access_GPU_Read); //???staging to?
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) Access.Set(Access_CPU_Read);
	if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) Access.Set(Access_CPU_Write);
	if (D3DDesc.Usage == D3D11_USAGE_DEFAULT || D3DDesc.Usage == D3D11_USAGE_STAGING) Access.Set(Access_GPU_Write); //???staging to?

	VertexLayout = &Layout;
	VertexCount = D3DDesc.ByteWidth / VertexSize;
	pBuffer = pVB;
	D3DUsage = D3DDesc.Usage;

	OK;
}
//---------------------------------------------------------------------

void CD3D11VertexBuffer::InternalDestroy()
{
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

}