#include "D3D9GPUDriver.h"

#include <Render/D3D9/D3D9Fwd.h>
#include <Render/D3D9/D3D9DriverFactory.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CD3D9GPUDriver, 'D9GD', Render::CGPUDriver);

bool CD3D9GPUDriver::CheckCaps(ECaps Cap)
{
	n_assert(pD3DDevice);

	switch (Cap)
	{
		case Caps_VSTexFiltering_Linear:
			return (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) && (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR);
		case Caps_VSTex_L16:
			return SUCCEEDED(D3D9DrvFactory->GetDirect3D9()->CheckDeviceFormat(	Adapter,
																				DEM_D3D_DEVICETYPE,
																				D3DFMT_UNKNOWN, //D3DPresentParams.BackBufferFormat,
																				D3DUSAGE_QUERY_VERTEXTEXTURE,
																				D3DRTYPE_TEXTURE,
																				D3DFMT_L16));
		default: FAIL;
	}
}
//---------------------------------------------------------------------

// If device exists, create additional swap chain. If device not exist, create device with implicit swap chain.
DWORD CD3D9GPUDriver::CreateSwapChain(const CSwapChainDesc& Desc, const Sys::COSWindow* pWindow)
{
	if (pD3DDevice)
	{
		// create additional swap chain
		// return its index
		return ERR_MAX_SWAP_CHAIN_COUNT_EXCEEDED;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };

#if DEM_D3D_USENVPERFHUD
	//Adapter = D3D9DrvFactory->GetAdapterCount() - 1;
#endif

	n_assert(D3D9DrvFactory->AdapterExists(Adapter));

	IDirect3D9* pD3D9 = D3D9DrvFactory->GetDirect3D9();

	memset(&D3DCaps, 0, sizeof(D3DCaps));
	n_assert(SUCCEEDED(pD3D9->GetDeviceCaps(Adapter, DEM_D3D_DEVICETYPE, &D3DCaps)));

	// D3DPRESENT_INTERVAL_ONE - as _DEFAULT, but improves VSync quality at a little cost of processing time
	D3DPresentParams.PresentationInterval = Desc.Flags.Is(SwapChain_VSync) ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;

	DWORD BufferCount = Desc.BufferCount ? Desc.BufferCount : 2;

	switch (Desc.SwapMode)
	{
		case SwapMode_CopyPersist:	D3DPresentParams.SwapEffect = BufferCount > 1 ? D3DSWAPEFFECT_FLIP : D3DSWAPEFFECT_COPY; break;
		//case SwapMode_FlipPersist:	// FlipEx, but it is available only in D3D9Ex
		case SwapMode_CopyDiscard:
		default:					D3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD; break; // Allow runtime to select the best
	}

	// It is recommended to use non-MSAA swap chain, render to MSAA RT and then resolve it to the back buffer
	if (D3DPresentParams.SwapEffect == D3DSWAPEFFECT_DISCARD)
	{
		//???support MSAA or enforce separate MSAA RT?
		D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
		D3DPresentParams.MultiSampleQuality = 0;
	}
	else
	{
		D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
		D3DPresentParams.MultiSampleQuality = 0;
	}

	D3DPresentParams.hDeviceWindow = Display.GetAppHwnd();
	D3DPresentParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

	//start SetupPresentParams();
	if (Display.Fullscreen)
	{
		D3DPresentParams.BackBufferCount = Display.TripleBuffering ? 2 : 1;
		D3DPresentParams.Windowed = Display.DisplayModeSwitchEnabled ? FALSE : TRUE;
	}
	else
	{
		D3DPresentParams.BackBufferCount = 1;
		D3DPresentParams.Windowed = TRUE;
	}

	CDisplayMode DisplayMode = Display.GetDisplayMode();
	if (D3DPresentParams.Windowed)
	{
		// Uses current desktop mode
		DisplayMode.PixelFormat = D3DFMT_UNKNOWN;
	}
	else
	{
		if (DisplayMode.PixelFormat == D3DFMT_UNKNOWN)
			DisplayMode.PixelFormat = D3DFMT_X8R8G8B8;

		CArray<CDisplayMode> Modes;
		Display.GetAvailableDisplayModes((CDisplay::DWORD)D3DAdapter, D3DPresentParams.BackBufferFormat, Modes);
		if (Modes.FindIndex(DisplayMode) == INVALID_INDEX)
		{
			// Find available mode the most close to the requested one
			float IdealAspect = Display.GetDisplayMode().GetAspectRatio();
			float IdealResolution = (float)Display.GetDisplayMode().Width * (float)Display.GetDisplayMode().Height;
			float MinMetric = FLT_MAX;
			int MinIdx = INVALID_INDEX;
			for (int i = 0; i < Modes.GetCount(); ++i)
			{
				const CDisplayMode& Mode = Modes[i];
				float AspectDiff = Mode.GetAspectRatio() - IdealAspect;
				float ResolutionDiff = (float)(Mode.Width * Mode.Height) - IdealResolution;
				float Metric = AspectDiff * AspectDiff + ResolutionDiff * ResolutionDiff;
				if (Metric < MinMetric)
				{
					MinMetric = Metric;
					MinIdx = i;
				}
			}
			n_assert(MinIdx != INVALID_INDEX);
			DisplayMode.Width = Modes[MinIdx].Width;
			DisplayMode.Height = Modes[MinIdx].Height;
		}
	}

	Display.SetDisplayMode(DisplayMode);

	// Make sure the device supports a D24S8 depth buffers
	HRESULT hr = pD3D->CheckDeviceFormat(	D3DAdapter,
											DEM_D3D_DEVICETYPE,
											DisplayMode.PixelFormat,
											D3DUSAGE_DEPTHSTENCIL,
											D3DRTYPE_SURFACE,
											D3DFMT_D24S8);
	if (FAILED(hr))
	{
		Sys::Error("Rendering device doesn't support D24S8 depth buffer!\n");
		return;
	}

	// Check that the depth buffer format is compatible with the backbuffer format
	hr = pD3D->CheckDepthStencilMatch(	D3DAdapter,
										DEM_D3D_DEVICETYPE,
										DisplayMode.PixelFormat,
										DisplayMode.PixelFormat,
										D3DFMT_D24S8);
	if (FAILED(hr))
	{
		Sys::Error("Backbuffer format is not compatible with D24S8 depth buffer!\n");
		return;
	}

	D3DPresentParams.BackBufferWidth = DisplayMode.Width;
	D3DPresentParams.BackBufferHeight = DisplayMode.Height;
	D3DPresentParams.BackBufferFormat = DisplayMode.PixelFormat;
	D3DPresentParams.EnableAutoDepthStencil = TRUE;
	D3DPresentParams.AutoDepthStencilFormat = D3DFMT_D24S8;
	D3DPresentParams.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
	//end SetupPresentParams();

#if DEM_D3D_DEBUG
	D3DPresentParams.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	DWORD BhvFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#else
	DWORD BhvFlags = D3DCREATE_FPU_PRESERVE;
	BhvFlags |= (D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
		D3DCREATE_HARDWARE_VERTEXPROCESSING :
		D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#endif

	// NB: May fail if can't create requested number of backbuffers
	HRESULT hr = pD3D9->CreateDevice(Adapter,
									 DEM_D3D_DEVICETYPE,
									 Display.GetAppHwnd(),	//!!!focus window to react on alt-tab, one for all devices!
									 BhvFlags,
									 &D3DPresentParams,
									 &pD3DDevice);

	if (FAILED(hr))
	{
		Sys::Error("Failed to create Direct3D device object: %s!\n", DXGetErrorString(hr));
		FAIL;
	}

	//start SetupDevice();
	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

	D3DVIEWPORT9 ViewPort;
	ViewPort.Width = D3DPresentParams.BackBufferWidth;
	ViewPort.Height = D3DPresentParams.BackBufferHeight;
	ViewPort.X = 0;
	ViewPort.Y = 0;
	ViewPort.MinZ = 0.0f;
	ViewPort.MaxZ = 1.0f;
	pD3DDevice->SetViewport(&ViewPort);

	CurrDepthStencilFormat =
		D3DPresentParams.EnableAutoDepthStencil ?  D3DPresentParams.AutoDepthStencilFormat : D3DFMT_UNKNOWN;

	//!!!CreateQueries();
	//end SetupDevice();

	return 0;
}
//---------------------------------------------------------------------

}