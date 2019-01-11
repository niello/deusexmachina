#include "D3D9DriverFactory.h"

#include <Render/D3D9/D3D9DisplayDriver.h>
#include <Render/D3D9/D3D9GPUDriver.h>

#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h>

namespace Render
{
__ImplementClassNoFactory(Render::CD3D9DriverFactory, Render::CVideoDriverFactory);
__ImplementSingleton(Render::CD3D9DriverFactory);

bool CD3D9DriverFactory::Create()
{
	Release();

	pD3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D9) FAIL;
	AdapterCount = pD3D9->GetAdapterCount();
	OK;
}
//---------------------------------------------------------------------

void CD3D9DriverFactory::Release()
{
	AdapterCount = 0;
	SAFE_RELEASE(pD3D9);
}
//---------------------------------------------------------------------

bool CD3D9DriverFactory::GetAdapterInfo(UPTR Adapter, CAdapterInfo& OutInfo) const
{
	if (Adapter >= AdapterCount) FAIL;
	D3DADAPTER_IDENTIFIER9 D3DAdapterInfo;
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

PDisplayDriver CD3D9DriverFactory::CreateDisplayDriver(UPTR Adapter, UPTR Output)
{
	// No support for multihead devices, you may add it if you need
	n_assert2(Output == 0, "D3D9 supports only one output (0) per video adapter");
	PD3D9DisplayDriver Driver = n_new(CD3D9DisplayDriver);
	if (!Driver->Init(Adapter, Output)) Driver = NULL;
	return Driver.Get();
}
//---------------------------------------------------------------------

PGPUDriver CD3D9DriverFactory::CreateGPUDriver(UPTR Adapter, EGPUDriverType DriverType)
{
	n_assert(pD3D9);

//???to virtual CreateNVidiaPerfHUDDriver(), determine adapter and type and proced here?
//or Adapter_NVPerfHUD and forced GPU_Reference?
#if DEM_RENDER_USENVPERFHUD
	for (UINT CurrAdapter = 0; CurrAdapter < pD3D9->GetAdapterCount(); ++CurrAdapter) 
	{ 
		D3DADAPTER_IDENTIFIER9 Identifier;
		if (SUCCEEDED(pD3D9->GetAdapterIdentifier(CurrAdapter, 0, &Identifier)) &&
			strstr(Identifier.Description, "PerfHUD") != 0)
		{
			Adapter = CurrAdapter;
			DriverType = GPU_Reference;
			break;
		}
	}
#endif

	if (Adapter == Adapter_AutoSelect && DriverType == GPU_AutoSelect) Adapter = 0;

	// Since device is actually created only in CD3D9GPUDriver::CreateSwapChain(), creation logic is moved there
	PD3D9GPUDriver Driver = n_new(CD3D9GPUDriver);
	if (!Driver->Init(Adapter, DriverType)) Driver = nullptr;
	return Driver;
}
//---------------------------------------------------------------------

D3DFORMAT CD3D9DriverFactory::PixelFormatToD3DFormat(EPixelFormat Format)
{
	switch (Format)
	{
		case PixelFmt_R8G8B8X8:					return D3DFMT_X8B8G8R8;
		case PixelFmt_R8G8B8A8:					return D3DFMT_A8B8G8R8;
		case PixelFmt_DefaultBackBuffer:
		case PixelFmt_B8G8R8X8:					return D3DFMT_X8R8G8B8;
		case PixelFmt_B8G8R8A8:					return D3DFMT_A8R8G8B8;
		case PixelFmt_B5G6R5:					return D3DFMT_R5G6B5;
		case PixelFmt_R8:						return D3DFMT_L8;
		case PixelFmt_R16:						return D3DFMT_L16;
		case PixelFmt_DXT1:						return D3DFMT_DXT1;
		case PixelFmt_DXT3:						return D3DFMT_DXT3;
		case PixelFmt_DXT5:						return D3DFMT_DXT5;
		case PixelFmt_DefaultDepthBuffer:
		case PixelFmt_D24:						return D3DFMT_D24X8;
		case PixelFmt_DefaultDepthStencilBuffer:
		case PixelFmt_D24S8:					return D3DFMT_D24S8;
		case PixelFmt_D32:						return D3DFMT_D32;
		case PixelFmt_Invalid:
		default:								return D3DFMT_UNKNOWN;
	}
}
//---------------------------------------------------------------------

EPixelFormat CD3D9DriverFactory::D3DFormatToPixelFormat(D3DFORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case D3DFMT_X8B8G8R8:	return PixelFmt_R8G8B8X8;
		case D3DFMT_A8B8G8R8:	return PixelFmt_R8G8B8A8;
		case D3DFMT_X8R8G8B8:	return PixelFmt_B8G8R8X8;
		case D3DFMT_A8R8G8B8:	return PixelFmt_B8G8R8A8;
		case D3DFMT_R5G6B5:		return PixelFmt_B5G6R5;
		case D3DFMT_L8:			return PixelFmt_R8;
		case D3DFMT_L16:		return PixelFmt_R16;
		case D3DFMT_DXT1:		return PixelFmt_DXT1;
		case D3DFMT_DXT3:		return PixelFmt_DXT3;
		case D3DFMT_DXT5:		return PixelFmt_DXT5;
		case D3DFMT_D24X8:		return PixelFmt_D24;
		case D3DFMT_D24S8:		return PixelFmt_D24S8;
		case D3DFMT_D32:		return PixelFmt_D32;
		case D3DFMT_UNKNOWN:
		default:				return PixelFmt_Invalid;
	}
}
//---------------------------------------------------------------------

UPTR CD3D9DriverFactory::D3DFormatBitsPerPixel(D3DFORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case D3DFMT_DXT1:			return 4;

		case D3DFMT_P8:
		case D3DFMT_A8:
		case D3DFMT_L8:
		case D3DFMT_R3G3B2:
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:			return 8;

		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_A4R4G4B4:
		case D3DFMT_A8R3G3B2:
		case D3DFMT_X4R4G4B4:
		case D3DFMT_A8P8:
		case D3DFMT_A8L8:
		case D3DFMT_V8U8:
		case D3DFMT_L6V5U5:
		case D3DFMT_L16:
		case D3DFMT_R16F:			return 16;

		case D3DFMT_R8G8B8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
		case D3DFMT_G16R16:
		case D3DFMT_A4L4:
		case D3DFMT_X8L8V8U8:
		case D3DFMT_Q8W8V8U8:
		case D3DFMT_V16U16:
		case D3DFMT_A2B10G10R10:
		case D3DFMT_A2W10V10U10:
		case D3DFMT_R32F:
		case D3DFMT_G16R16F:		return 32;

		case D3DFMT_A16B16G16R16F:
		case D3DFMT_G32R32F:		return 64;

		case D3DFMT_A32B32G32R32F:	return 128;

		default:					return 0;
	}
}
//---------------------------------------------------------------------

UPTR CD3D9DriverFactory::D3DFormatBlockSize(D3DFORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case D3DFMT_DXT1:
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:	return 4;
		default:			return 1;
	}
}
//---------------------------------------------------------------------

UPTR CD3D9DriverFactory::D3DFormatDepthBits(D3DFORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case D3DFMT_D15S1:			return 15;
		case D3DFMT_D16:
		case D3DFMT_D16_LOCKABLE:	return 16;
		case D3DFMT_D24X8:
		case D3DFMT_D24X4S4:
		case D3DFMT_D24S8:
		case D3DFMT_D24FS8:			return 24;
		case D3DFMT_D32:
		case D3DFMT_D32F_LOCKABLE:	return 32;
		default:					return 0;
	}
}
//---------------------------------------------------------------------

UPTR CD3D9DriverFactory::D3DFormatStencilBits(D3DFORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case D3DFMT_D15S1:		return 1;
		case D3DFMT_D24X4S4:	return 4;
		case D3DFMT_D24S8:
		case D3DFMT_D24FS8:		return 8;
		default:				return 0;
	}
}
//---------------------------------------------------------------------

EMSAAQuality CD3D9DriverFactory::D3DMSAAParamsToMSAAQuality(D3DMULTISAMPLE_TYPE MultiSampleType, UPTR MultiSampleQuality)
{
	switch (MultiSampleType)
	{
		case D3DMULTISAMPLE_NONE:		return MSAA_None;
		case D3DMULTISAMPLE_2_SAMPLES:	return MSAA_2x;
		case D3DMULTISAMPLE_4_SAMPLES:	return MSAA_4x;
		case D3DMULTISAMPLE_8_SAMPLES:	return MSAA_8x;
		default:						Sys::Error("CD3D9DriverFactory::D3DMSAAParamsToMSAAQuality() > Unsupported MSAA type %d", MultiSampleType); return MSAA_None;
	}
}
//---------------------------------------------------------------------

}
