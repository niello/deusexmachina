#include "D3D11DriverFactory.h"

#include <Render/D3D11/D3D11DisplayDriver.h>
//#include <Render/D3D11/D3D11GPUDriver.h>

namespace Render
{

bool CD3D11DriverFactory::Open()
{
	if (!SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pDXGIFactory))) FAIL;
	IDXGIAdapter1* pAdapter;
	while (pDXGIFactory->EnumAdapters1(AdapterCount, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		pAdapter->Release(); // AddRef() is called in EnumAdapters1()
		++AdapterCount;
	}
	OK;
}
//---------------------------------------------------------------------

void CD3D11DriverFactory::Close()
{
	AdapterCount = 0;
	SAFE_RELEASE(pDXGIFactory);
}
//---------------------------------------------------------------------

bool CD3D11DriverFactory::GetAdapterInfo(DWORD Adapter, CAdapterInfo& OutInfo) const
{
	if (Adapter >= AdapterCount) FAIL;

	IDXGIAdapter1* pAdapter;
	if (!SUCCEEDED(pDXGIFactory->EnumAdapters1(Adapter, &pAdapter))) FAIL;

	DXGI_ADAPTER_DESC1 D3DAdapterInfo = { 0 };
	bool Success = SUCCEEDED(pAdapter->GetDesc1(&D3DAdapterInfo));

	pAdapter->Release(); // AddRef() is called in EnumAdapters1()

	if (!Success) FAIL;

	//WideCharToMultiByte(
	//OutInfo.Description = D3DAdapterInfo.Description;
	OutInfo.Description = "";

	OutInfo.VendorID = D3DAdapterInfo.VendorId;
	OutInfo.DeviceID = D3DAdapterInfo.DeviceId;
	OutInfo.SubSysID = D3DAdapterInfo.SubSysId;
	OutInfo.Revision = D3DAdapterInfo.Revision;
	OutInfo.VideoMemBytes = D3DAdapterInfo.DedicatedVideoMemory;
	OutInfo.DedicatedSystemMemBytes = D3DAdapterInfo.DedicatedSystemMemory;
	OutInfo.SharedSystemMemBytes = D3DAdapterInfo.SharedSystemMemory;
	OutInfo.IsSoftware = ((D3DAdapterInfo.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0);

	OK;
}
//---------------------------------------------------------------------

PDisplayDriver CD3D11DriverFactory::CreateDisplayDriver(DWORD Adapter, DWORD Output)
{
	//return n_new(CD3D11DisplayDriver);
	return NULL;
}
//---------------------------------------------------------------------

PGPUDriver CD3D11DriverFactory::CreateGPUDriver(DWORD Adapter, bool CreateSwapChain, Sys::COSWindow* pWindow, CDisplayDriver* pFullscreenOutput)
{
	n_assert(!pFullscreenOutput || Adapter == pFullscreenOutput->GetAdapterID()); //???is necessary?

	// if !pWindow - use focus window (global application window)
	// if !pFullscreenOutput - don't restrict, use default output from window
	//return n_new(CD3D11GPUDriver);
	return NULL;
}
//---------------------------------------------------------------------

}
