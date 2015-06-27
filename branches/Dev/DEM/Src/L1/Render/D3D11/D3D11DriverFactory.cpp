#include "D3D11DriverFactory.h"

#include <Render/D3D11/D3D11DisplayDriver.h>
#include <Render/D3D11/D3D11GPUDriver.h>

#define WIN32_LEAN_AND_MEAN
#include <DXGI.h>

namespace Render
{
__ImplementClassNoFactory(Render::CD3D11DriverFactory, Render::CVideoDriverFactory);
__ImplementSingleton(Render::CD3D11DriverFactory);

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

DWORD CD3D11DriverFactory::GetAdapterOutputCount(DWORD Adapter) const
{
	if (Adapter >= AdapterCount) FAIL;

	IDXGIAdapter1* pAdapter;
	if (!SUCCEEDED(pDXGIFactory->EnumAdapters1(Adapter, &pAdapter))) FAIL;

	UINT OutputCount = 0;
	IDXGIOutput* pOutput;
	while (pAdapter->EnumOutputs(OutputCount, &pOutput) != DXGI_ERROR_NOT_FOUND)
	{
		pOutput->Release(); // AddRef() is called in EnumOutputs()
		++OutputCount;
	}

	pAdapter->Release(); // AddRef() is called in EnumAdapters1()

	return OutputCount;
}
//---------------------------------------------------------------------

PDisplayDriver CD3D11DriverFactory::CreateDisplayDriver(DWORD Adapter, DWORD Output)
{
	PD3D11DisplayDriver Driver = n_new(CD3D11DisplayDriver);
	if (!Driver->Init(Adapter, Output)) Driver = NULL;
	return Driver.GetUnsafe();
}
//---------------------------------------------------------------------

PDisplayDriver CD3D11DriverFactory::CreateDisplayDriver(IDXGIOutput* pOutput)
{
	if (!pOutput) return NULL;
	PD3D11DisplayDriver Driver = n_new(CD3D11DisplayDriver);
	Driver->pDXGIOutput = pOutput;
	//???determine adapter and output?
	//if (!Driver->Init(Adapter, Output)) Driver = NULL;
	return Driver.GetUnsafe();
}
//---------------------------------------------------------------------

// If adapter is specified, driver type will be automatically set to the type of that adapter.
// If adapter is not specified, adapter will be selected automatically.
PGPUDriver CD3D11DriverFactory::CreateGPUDriver(DWORD Adapter, EGPUDriverType DriverType)
{
	n_assert(pDXGIFactory);

//???to virtual CreateNVidiaPerfHUDDriver(), determine adapter and type and proced here?
//or Adapter_NVPerfHUD and forced GPU_Reference?
#if DEM_RENDER_USENVPERFHUD
	UINT CurrAdapter = 0;
	IDXGIAdapter1* pAdapter;
	while (pDXGIFactory->EnumAdapters1(CurrAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC Desc;
		if (SUCCEEDED(pAdapter->GetDesc(&Desc)) && wcscmp(Desc.Description, L"NVIDIA PerfHUD") == 0)
		{
			Adapter = CurrAdapter;
			DriverType = GPU_Reference;
			break;
		}
		pAdapter->Release(); // AddRef() is called in EnumAdapters1()
		++CurrAdapter;
	}
#endif

	PD3D11GPUDriver Driver = n_new(CD3D11GPUDriver);
	if (!Driver->Init(Adapter, DriverType)) Driver = NULL;
	return Driver.GetUnsafe();
}
//---------------------------------------------------------------------

DXGI_FORMAT CD3D11DriverFactory::PixelFormatToDXGIFormat(EPixelFormat Format)
{
	switch (Format)
	{
		case PixelFmt_DefaultBackBuffer:
		case PixelFmt_R8G8B8A8:				return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFmt_B8G8R8X8:				return DXGI_FORMAT_B8G8R8X8_UNORM; //DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case PixelFmt_B8G8R8A8:				return DXGI_FORMAT_B8G8R8A8_UNORM; //DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case PixelFmt_B5G6R5:				return DXGI_FORMAT_B5G6R5_UNORM;
		case PixelFmt_DXT1:					return DXGI_FORMAT_BC1_UNORM;
		case PixelFmt_DXT3:					return DXGI_FORMAT_BC2_UNORM;
		case PixelFmt_DXT5:					return DXGI_FORMAT_BC3_UNORM;
		case PixelFmt_Invalid:
		default:							return DXGI_FORMAT_UNKNOWN;
	}
}
//---------------------------------------------------------------------

EPixelFormat CD3D11DriverFactory::DXGIFormatToPixelFormat(DXGI_FORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:	return PixelFmt_R8G8B8A8;
		case DXGI_FORMAT_B8G8R8X8_UNORM:	return PixelFmt_B8G8R8X8; //DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case DXGI_FORMAT_B8G8R8A8_UNORM:	return PixelFmt_B8G8R8A8; //DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case DXGI_FORMAT_B5G6R5_UNORM:		return PixelFmt_B5G6R5;
		case DXGI_FORMAT_BC1_UNORM:			return PixelFmt_DXT1;
		case DXGI_FORMAT_BC2_UNORM:			return PixelFmt_DXT3;
		case DXGI_FORMAT_BC3_UNORM:			return PixelFmt_DXT5;
		case DXGI_FORMAT_UNKNOWN:
		default:							return PixelFmt_Invalid;
	}
}
//---------------------------------------------------------------------

EMSAAQuality CD3D11DriverFactory::D3DMSAAParamsToMSAAQuality(DXGI_SAMPLE_DESC SampleDesc)
{
	switch (SampleDesc.Count)
	{
		case 0:
		case 1:		return MSAA_None;
		case 2:		return MSAA_2x;
		case 4:		return MSAA_4x;
		case 8:		return MSAA_8x;
		default:	Sys::Error("CD3D11DriverFactory::D3DMSAAParamsToMSAAQuality() > Unsupported MSAA type %d", SampleDesc.Count); return MSAA_None;
	}
}
//---------------------------------------------------------------------

}
