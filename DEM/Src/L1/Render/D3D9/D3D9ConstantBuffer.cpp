#include "D3D9ConstantBuffer.h"

#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CD3D9ConstantBuffer, 'CB09', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D9ConstantBuffer::Create(/*ID3D11Buffer* pCB, ID3D11DeviceContext* pD3DDeviceCtx, bool StoreRAMCopy*/)
{
	//if (!pCB || !pD3DDeviceCtx) FAIL;

	OK;
}
//---------------------------------------------------------------------

void CD3D9ConstantBuffer::InternalDestroy()
{
}
//---------------------------------------------------------------------

bool CD3D9ConstantBuffer::BeginChanges()
{
	OK;
}
//---------------------------------------------------------------------

bool CD3D9ConstantBuffer::CommitChanges()
{
	OK;
}
//---------------------------------------------------------------------

}
