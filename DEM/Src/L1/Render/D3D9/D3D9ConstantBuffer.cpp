#include "D3D9ConstantBuffer.h"

#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CD3D9ConstantBuffer, 'CB09', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D9ConstantBuffer::Create()
{
	//if (!pCB || !pD3DDeviceCtx) FAIL;

	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

void CD3D9ConstantBuffer::InternalDestroy()
{
	NOT_IMPLEMENTED;
}
//---------------------------------------------------------------------

bool CD3D9ConstantBuffer::BeginChanges()
{
	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9ConstantBuffer::SetFloat(DWORD Offset, const float* pData, DWORD Count)
{
	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9ConstantBuffer::SetInt(DWORD Offset, const int* pData, DWORD Count)
{
	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9ConstantBuffer::CommitChanges()
{
	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

}
