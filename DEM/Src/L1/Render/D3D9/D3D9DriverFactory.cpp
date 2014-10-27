#include "D3D9DriverFactory.h"

#include <Render/D3D9/D3D9DisplayDriver.h>
//#include <Render/D3D9/D3D9GPUDriver.h>

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
	if (Adapter >= AdapterCount) FAIL;
	D3DADAPTER_IDENTIFIER9 D3DAdapterInfo = { 0 }; // NB: there is a constructible object CSimpleString in this structure
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
	//???!!!what about multihead?! D3D9 additional swap chains can access different outputs.
	//Disallow D3D9 multimonitor for now?
	n_assert2(Output == 0, "D3D9 supports only one output (0) per video adapter");
	PD3D9DisplayDriver Driver = n_new(CD3D9DisplayDriver);
	if (!Driver->Init(Adapter, Output)) Driver = NULL;
	return Driver.GetUnsafe();
}
//---------------------------------------------------------------------

PGPUDriver CD3D9DriverFactory::CreateGPUDriver(DWORD Adapter, bool CreateSwapChain, Sys::COSWindow* pWindow, CDisplayDriver* pFullscreenOutput)
{
	n_assert2(!CreateSwapChain, "D3D9 device can't be created without a swap chain");

	// if !pWindow - use focus window (global application window)
	// if !pFullscreenOutput - don't restrict, use default output from window
	//return n_new(CD3D9GPUDriver);

	//???call init (which is empty for D3D9) and then call createswapchain right here?

	return NULL;
}
//---------------------------------------------------------------------

D3DFORMAT CD3D9DriverFactory::PixelFormatToD3DFormat(EPixelFormat Format)
{
	switch (Format)
	{
		case PixelFmt_X8R8G8B8:	return D3DFMT_X8R8G8B8;
		case PixelFmt_A8R8G8B8:	return D3DFMT_A8R8G8B8;
		case PixelFmt_R5G6B5:	return D3DFMT_R5G6B5;
		case PixelFmt_Invalid:
		default:				return D3DFMT_UNKNOWN;
	}
}
//---------------------------------------------------------------------

EPixelFormat CD3D9DriverFactory::D3DFormatToPixelFormat(D3DFORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case D3DFMT_X8R8G8B8:	return PixelFmt_X8R8G8B8;
		case D3DFMT_A8R8G8B8:	return PixelFmt_A8R8G8B8;
		case D3DFMT_R5G6B5:		return PixelFmt_R5G6B5;
		case D3DFMT_UNKNOWN:
		default:				return PixelFmt_Invalid;
	}
}
//---------------------------------------------------------------------

}
