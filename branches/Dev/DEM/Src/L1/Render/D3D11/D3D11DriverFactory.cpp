#include "D3D11DriverFactory.h"

#include <Render/D3D11/D3D11DisplayDriver.h>
//#include <Render/D3D11/D3D11GPUDriver.h>

#define WIN32_LEAN_AND_MEAN
#include <DXGI.h>

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

	D3DAdapterInfo.Description[sizeof_array(D3DAdapterInfo.Description) - 1] = 0;
	int Len = WideCharToMultiByte(CP_UTF8, 0, D3DAdapterInfo.Description, sizeof_array(D3DAdapterInfo.Description), NULL, 0, NULL, NULL);
	if (Len > 0)
	{
		char* pBuf = (char*)_malloca(Len); // Len includes terminating NULL
		WideCharToMultiByte(CP_UTF8, 0, D3DAdapterInfo.Description, sizeof_array(D3DAdapterInfo.Description), pBuf, Len, NULL, NULL);
		OutInfo.Description = pBuf;
		_freea(pBuf);
	}

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
	PD3D11DisplayDriver Driver = n_new(CD3D11DisplayDriver);
	if (!Driver->Init(Adapter, Output)) Driver = NULL;
	return Driver.GetUnsafe();
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

DXGI_FORMAT CD3D11DriverFactory::PixelFormatToDXGIFormat(EPixelFormat Format)
{
	switch (Format)
	{
		//???
		case PixelFmt_X8R8G8B8:	return DXGI_FORMAT_B8G8R8X8_UNORM; //DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case PixelFmt_A8R8G8B8:	return DXGI_FORMAT_B8G8R8A8_UNORM; //DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case PixelFmt_R5G6B5:	return DXGI_FORMAT_B5G6R5_UNORM;
		case PixelFmt_Invalid:
		default:				return DXGI_FORMAT_UNKNOWN;
	}
}
//---------------------------------------------------------------------

EPixelFormat CD3D11DriverFactory::DXGIFormatToPixelFormat(DXGI_FORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		//???
		case DXGI_FORMAT_B8G8R8X8_UNORM:	return PixelFmt_X8R8G8B8; //DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case DXGI_FORMAT_B8G8R8A8_UNORM:	return PixelFmt_A8R8G8B8; //DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case DXGI_FORMAT_B5G6R5_UNORM:		return PixelFmt_R5G6B5;
		case DXGI_FORMAT_UNKNOWN:
		default:							return PixelFmt_Invalid;
	}
}
//---------------------------------------------------------------------

}
