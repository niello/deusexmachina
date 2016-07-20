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

	for (UPTR i = 0; i < ShaderSignatures.GetCount(); ++i)
		n_free(ShaderSignatures[i].pData);
	ShaderSignatures.Clear();
	ShaderSigIDToIndex.Clear();
}
//---------------------------------------------------------------------

bool CD3D11DriverFactory::GetAdapterInfo(UPTR Adapter, CAdapterInfo& OutInfo) const
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

UPTR CD3D11DriverFactory::GetAdapterOutputCount(UPTR Adapter) const
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

PDisplayDriver CD3D11DriverFactory::CreateDisplayDriver(UPTR Adapter, UPTR Output)
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
PGPUDriver CD3D11DriverFactory::CreateGPUDriver(UPTR Adapter, EGPUDriverType DriverType)
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

// NB: Doesn't copy data, so pData must be dynamically allocated and must not be freed externally
bool CD3D11DriverFactory::RegisterShaderInputSignature(U32 ID, void* pData, UPTR Size)
{
	CBinaryData* pBinary = ShaderSignatures.Reserve(1);
	pBinary->pData = pData;
	pBinary->Size = Size;
	ShaderSigIDToIndex.Add(ID, ShaderSignatures.GetCount() - 1, true);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11DriverFactory::FindShaderInputSignature(UPTR ID, CBinaryData* pOutSigData) const
{
	UPTR Index;
	if (!ShaderSigIDToIndex.Get(ID, Index)) FAIL;
	if (pOutSigData) *pOutSigData = ShaderSignatures[Index];
	OK;
}
//---------------------------------------------------------------------

DXGI_FORMAT CD3D11DriverFactory::PixelFormatToDXGIFormat(EPixelFormat Format)
{
	switch (Format)
	{
		case PixelFmt_DefaultBackBuffer:
		case PixelFmt_R8G8B8A8:					return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFmt_B8G8R8X8:					return DXGI_FORMAT_B8G8R8X8_UNORM; //DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case PixelFmt_B8G8R8A8:					return DXGI_FORMAT_B8G8R8A8_UNORM; //DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case PixelFmt_B5G6R5:					return DXGI_FORMAT_B5G6R5_UNORM;
		case PixelFmt_R16:						return DXGI_FORMAT_R16_UNORM;
		case PixelFmt_DXT1:						return DXGI_FORMAT_BC1_UNORM;
		case PixelFmt_DXT3:						return DXGI_FORMAT_BC2_UNORM;
		case PixelFmt_DXT5:						return DXGI_FORMAT_BC3_UNORM;
		case PixelFmt_DefaultDepthStencilBuffer:
		case PixelFmt_D24S8:
		case PixelFmt_D24:						return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case PixelFmt_DefaultDepthBuffer:
		case PixelFmt_D32:						return DXGI_FORMAT_D32_FLOAT;
		case PixelFmt_D32S8:					return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case PixelFmt_Invalid:
		default:								return DXGI_FORMAT_UNKNOWN;
	}
}
//---------------------------------------------------------------------

EPixelFormat CD3D11DriverFactory::DXGIFormatToPixelFormat(DXGI_FORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case DXGI_FORMAT_R8G8B8A8_UNORM:		return PixelFmt_R8G8B8A8;
		case DXGI_FORMAT_B8G8R8X8_UNORM:		return PixelFmt_B8G8R8X8; //DXGI_FORMAT_B8G8R8X8_UNORM_SRGB
		case DXGI_FORMAT_B8G8R8A8_UNORM:		return PixelFmt_B8G8R8A8; //DXGI_FORMAT_B8G8R8A8_UNORM_SRGB
		case DXGI_FORMAT_B5G6R5_UNORM:			return PixelFmt_B5G6R5;
		case DXGI_FORMAT_BC1_UNORM:				return PixelFmt_DXT1;
		case DXGI_FORMAT_BC2_UNORM:				return PixelFmt_DXT3;
		case DXGI_FORMAT_BC3_UNORM:				return PixelFmt_DXT5;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:		return PixelFmt_D24S8;
		case DXGI_FORMAT_D32_FLOAT:				return PixelFmt_D32;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:	return PixelFmt_D32S8;
		case DXGI_FORMAT_UNKNOWN:
		default:								return PixelFmt_Invalid;
	}
}
//---------------------------------------------------------------------

UPTR CD3D11DriverFactory::DXGIFormatBitsPerPixel(DXGI_FORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		// 1 bpp
		case DXGI_FORMAT_R1_UNORM:						return 1;

		// 4 bpp
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:						return 4;

		// 8 bpp
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_A8_UNORM:						return 8;

		// 16 bpp
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_B5G6R5_UNORM:
		case DXGI_FORMAT_B5G5R5A1_UNORM:
		case DXGI_FORMAT_B4G4R4A4_UNORM:				return 16;

		// 32 bpp
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		case DXGI_FORMAT_R11G11B10_FLOAT:
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
		case DXGI_FORMAT_R8G8_B8G8_UNORM:
		case DXGI_FORMAT_G8R8_G8B8_UNORM:
		case DXGI_FORMAT_B8G8R8A8_UNORM:
		case DXGI_FORMAT_B8G8R8X8_UNORM:
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:			return 32;

		// 64 bpp
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:		return 64;

		// 96 bpp
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:				return 96;

		// 128 bpp
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:				return 128;

		default:										return 0;
	}
}
//---------------------------------------------------------------------

UPTR CD3D11DriverFactory::DXGIFormatDepthBits(DXGI_FORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case DXGI_FORMAT_D16_UNORM:				return 16;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:		return 24;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_D32_FLOAT:				return 32;
		default:								return 0;
	}
}
//---------------------------------------------------------------------

UPTR CD3D11DriverFactory::DXGIFormatStencilBits(DXGI_FORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:	return 8;
		default:								return 0;
	}
}
//---------------------------------------------------------------------

UPTR CD3D11DriverFactory::DXGIFormatBlockSize(DXGI_FORMAT D3DFormat)
{
	switch (D3DFormat)
	{
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:	return 4;

		default:							return 1;
	}
}
//---------------------------------------------------------------------

DXGI_FORMAT CD3D11DriverFactory::GetCorrespondingFormat(DXGI_FORMAT D3DFormat, EFormatType Type, bool PreferSRGB)
{
	switch (D3DFormat)
	{
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
		case DXGI_FORMAT_D16_UNORM:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R16_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UInt:			return DXGI_FORMAT_R16_UINT;
				case FmtType_UNorm:			return DXGI_FORMAT_R16_UNORM;
				case FmtType_SInt:			return DXGI_FORMAT_R16_SINT;
				case FmtType_SNorm:			return DXGI_FORMAT_R16_SNORM;
				case FmtType_Float:			return DXGI_FORMAT_R16_FLOAT;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
		case DXGI_FORMAT_D32_FLOAT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R32_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R32_UINT;
				case FmtType_SInt:			return DXGI_FORMAT_R32_SINT;
				case FmtType_DefaultTyped:
				case FmtType_Float:			return DXGI_FORMAT_R32_FLOAT;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_BC1_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return PreferSRGB ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_BC2_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return PreferSRGB ? DXGI_FORMAT_BC2_UNORM_SRGB : DXGI_FORMAT_BC2_UNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_BC3_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return PreferSRGB ? DXGI_FORMAT_BC3_UNORM_SRGB : DXGI_FORMAT_BC3_UNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_BC4_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_BC4_UNORM;
				case FmtType_SNorm:			return DXGI_FORMAT_BC4_SNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_BC5_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_BC5_UNORM;
				case FmtType_SNorm:			return DXGI_FORMAT_BC5_SNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
		{
			switch (Type)
			{
				// Not exact
				case FmtType_Typeless:		return DXGI_FORMAT_BC6H_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_BC6H_UF16;
				case FmtType_SNorm:
				case FmtType_Float:			return DXGI_FORMAT_BC6H_SF16;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_BC7_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return PreferSRGB ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R8_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R8_UINT;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_R8_UNORM;
				case FmtType_SInt:			return DXGI_FORMAT_R8_SINT;
				case FmtType_SNorm:			return DXGI_FORMAT_R8_SNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R8G8_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R8G8_UINT;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_R8G8_UNORM;
				case FmtType_SInt:			return DXGI_FORMAT_R8G8_SINT;
				case FmtType_SNorm:			return DXGI_FORMAT_R8G8_SNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R10G10B10A2_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R10G10B10A2_UINT;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_R10G10B10A2_UNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R8G8B8A8_UINT;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return PreferSRGB ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
				case FmtType_SInt:			return DXGI_FORMAT_R8G8B8A8_SINT;
				case FmtType_SNorm:			return DXGI_FORMAT_R8G8B8A8_SNORM;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R16G16_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R16G16_UINT;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_R16G16_UNORM;
				case FmtType_SInt:			return DXGI_FORMAT_R16G16_SINT;
				case FmtType_SNorm:			return DXGI_FORMAT_R16G16_SNORM;
				case FmtType_Float:			return DXGI_FORMAT_R16G16_FLOAT;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R24G8_TYPELESS:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R24G8_TYPELESS;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R16G16B16A16_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R16G16B16A16_UINT;
				case FmtType_DefaultTyped:
				case FmtType_UNorm:			return DXGI_FORMAT_R16G16B16A16_UNORM;
				case FmtType_SInt:			return DXGI_FORMAT_R16G16B16A16_SINT;
				case FmtType_SNorm:			return DXGI_FORMAT_R16G16B16A16_SNORM;
				case FmtType_Float:			return DXGI_FORMAT_R16G16B16A16_FLOAT;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R32G32_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R32G32_UINT;
				case FmtType_SInt:			return DXGI_FORMAT_R32G32_SINT;
				case FmtType_DefaultTyped:
				case FmtType_Float:			return DXGI_FORMAT_R32G32_FLOAT;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R32G32B32_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R32G32B32_UINT;
				case FmtType_SInt:			return DXGI_FORMAT_R32G32B32_SINT;
				case FmtType_DefaultTyped:
				case FmtType_Float:			return DXGI_FORMAT_R32G32B32_FLOAT;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
		{
			switch (Type)
			{
				case FmtType_Typeless:		return DXGI_FORMAT_R32G32B32A32_TYPELESS;
				case FmtType_UInt:			return DXGI_FORMAT_R32G32B32A32_UINT;
				case FmtType_SInt:			return DXGI_FORMAT_R32G32B32A32_SINT;
				case FmtType_DefaultTyped:
				case FmtType_Float:			return DXGI_FORMAT_R32G32B32A32_FLOAT;
				default:					return DXGI_FORMAT_UNKNOWN;
			}
		}

		default:							return DXGI_FORMAT_UNKNOWN;
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
