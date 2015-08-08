#include "D3D9ConstantBuffer.h"

#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClass(Render::CD3D9ConstantBuffer, 'CB09', Render::CConstantBuffer);

//!!!???assert destroyed?!
bool CD3D9ConstantBuffer::Create(/*pfloatregs, floatcount, pintregs, intcount*/)
{
	//if (!pCB || !pD3DDeviceCtx) FAIL;

	//!!!allocate float4/int4!
	//???or not float count but float4 count everywhere?

	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

void CD3D9ConstantBuffer::InternalDestroy()
{
	if (pFloatData)
	{
		n_free(pFloatData);
		pFloatData = NULL;
	}
	else if (pIntData)
	{
		n_free(pIntData);
	}
	pFloatRegisters = NULL;
	FloatCount = 0;
	pIntData = NULL;
	pIntRegisters = NULL;
	IntCount = 0;
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
	//pDevice->SetVertexShaderConstantF(StartReg, pdata, float4_count);
	//pDevice->SetVertexShaderConstantI(StartReg, pInt, int4_count);
	//pDevice->SetVertexShaderConstantB(StartReg, pBool, bool_count);
	NOT_IMPLEMENTED;
	OK;
}
//---------------------------------------------------------------------

}
