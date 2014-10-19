#include "D3D9DriverFactory.h"

#include <Render/D3D9/D3D9DisplayDriver.h>

#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{

bool CD3D9DriverFactory::Open()
{
	pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D9) FAIL;
	AdapterCount = pD3D9->GetAdapterCount();
	OK;
}
//---------------------------------------------------------------------

void CD3D9DriverFactory::Close()
{
	AdapterCount = 0;
	SAFE_RELEASE(pD3D9);
}
//---------------------------------------------------------------------

bool CD3D9DriverFactory::GetAdapterInfo(DWORD Adapter, CAdapterInfo& OutInfo) const
{
	n_assert(Adapter < AdapterCount);
	D3DADAPTER_IDENTIFIER9 D3DAdapterInfo = { 0 };
	if (!SUCCEEDED(pD3D9->GetAdapterIdentifier(Adapter, 0, &D3DAdapterInfo))) FAIL;
	OutInfo.Description = D3DAdapterInfo.Description;
	OutInfo.VendorID = D3DAdapterInfo.VendorId;
	OutInfo.DeviceID = D3DAdapterInfo.DeviceId;
	OutInfo.SubSysID = D3DAdapterInfo.SubSysId;
	OutInfo.Revision = D3DAdapterInfo.Revision;
	OutInfo.VideoMemBytes = 0;
	OutInfo.DedicatedSystemMemBytes = 0;
	OutInfo.SharedSystemMemBytes = 0;
	OutInfo.IsSoftware = false;
	OK;
}
//---------------------------------------------------------------------

PDisplayDriver CD3D9DriverFactory::CreateDisplayDriver(DWORD Adapter, DWORD Output)
{
	n_assert2(Output == 0, "D3D9 supports only one output (0) per video adapter");
	return n_new(CD3D9DisplayDriver);
}
//---------------------------------------------------------------------

}
