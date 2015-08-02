#include "D3D11ConstantBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11ConstantBuffer, 'CB11', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D11ConstantBuffer::Create(ID3D11Buffer* pCB)
{
	if (!pCB) FAIL;

	D3D11_BUFFER_DESC D3DDesc;
	pCB->GetDesc(&D3DDesc);

	if (!(D3DDesc.BindFlags & D3D11_BIND_CONSTANT_BUFFER)) FAIL;

	//???!!!can't be staging?!
	//Access.ResetTo(Access_GPU_Read); //???staging to?
	//if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) Access.Set(Access_CPU_Read);
	//if (D3DDesc.CPUAccessFlags & D3D11_CPU_ACCESS_WRITE) Access.Set(Access_CPU_Write);
	//if (D3DDesc.Usage == D3D11_USAGE_DEFAULT || D3DDesc.Usage == D3D11_USAGE_STAGING) Access.Set(Access_GPU_Write); //???staging to?

	pBuffer = pCB;
	D3DUsage = D3DDesc.Usage;

	OK;
}
//---------------------------------------------------------------------

void CD3D11ConstantBuffer::InternalDestroy()
{
	SAFE_RELEASE(pBuffer);
}
//---------------------------------------------------------------------

}
