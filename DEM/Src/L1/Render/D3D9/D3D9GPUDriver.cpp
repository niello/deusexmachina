#include "D3D9GPUDriver.h"

#include <Render/D3D9/D3D9Fwd.h>
#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9DisplayDriver.h>
#include <System/OSWindow.h>
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

void CD3D9GPUDriver::FillD3DPresentParams(const CSwapChainDesc& Desc, const Sys::COSWindow* pWindow, D3DPRESENT_PARAMETERS& D3DPresentParams)
{
	DWORD BufferCount = Desc.BufferCount ? Desc.BufferCount : 2;

	switch (Desc.SwapMode)
	{
		case SwapMode_CopyPersist:	D3DPresentParams.SwapEffect = BufferCount > 1 ? D3DSWAPEFFECT_FLIP : D3DSWAPEFFECT_COPY; break;
		//case SwapMode_FlipPersist:	// D3DSWAPEFFECT_FLIPEX, but it is available only in D3D9Ex
		case SwapMode_CopyDiscard:
		default:					D3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD; break; // Allows runtime to select the best
	}

	if (D3DPresentParams.SwapEffect == D3DSWAPEFFECT_DISCARD)
	{
		// It is recommended to use non-MSAA swap chain, render to MSAA RT and then resolve it to the back buffer
		//???support MSAA or enforce separate MSAA RT?
		D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
		D3DPresentParams.MultiSampleQuality = 0;
	}
	else
	{
		// MSAA not supported for swap effects other than D3DSWAPEFFECT_DISCARD
		D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
		D3DPresentParams.MultiSampleQuality = 0;
	}

	D3DPresentParams.hDeviceWindow = pWindow->GetHWND();
	D3DPresentParams.BackBufferCount = BufferCount - 1; //!!!N3 always sets 1 in windowed mode! why? //???in what swap effects should not substract 1?
	D3DPresentParams.EnableAutoDepthStencil = TRUE;
	D3DPresentParams.AutoDepthStencilFormat = D3DFMT_D24S8; // D3DFMT_D32 if no need in stencil

	// D3DPRESENT_INTERVAL_ONE - as _DEFAULT, but improves VSync quality at a little cost of processing time (uses another timer)
	D3DPresentParams.PresentationInterval = Desc.Flags.Is(SwapChain_VSync) ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	D3DPresentParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	D3DPresentParams.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
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

	//!!!n_assert(); there are no active swap chains!

#if DEM_D3D_USENVPERFHUD
	Adapter = D3D9DrvFactory->GetAdapterCount() - 1; // NVPerfHUD adapter //???is always the last, or read adapter info?
#endif

	n_assert(D3D9DrvFactory->AdapterExists(Adapter));

	IDirect3D9* pD3D9 = D3D9DrvFactory->GetDirect3D9();

	memset(&D3DCaps, 0, sizeof(D3DCaps));
	n_assert(SUCCEEDED(pD3D9->GetDeviceCaps(Adapter, DEM_D3D_DEVICETYPE, &D3DCaps)));

	const Sys::COSWindow* pWnd = pWindow ? pWindow : D3D9DrvFactory->GetFocusWindow();

	//???why it is better to create windowed swap chain and then turn it fullscreen? except that it uses less aruments on creation.
	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(Desc, pWnd, D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = pWnd->GetWidth();
	D3DPresentParams.BackBufferHeight = pWnd->GetHeight();
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	// Make sure the device supports a depth buffer specified
	//???does support D3DFMT_UNKNOWN? if not, need to explicitly determine adapter format.
	HRESULT hr = pD3D9->CheckDeviceFormat(	Adapter,
											DEM_D3D_DEVICETYPE,
											D3DPresentParams.BackBufferFormat,
											D3DUSAGE_DEPTHSTENCIL,
											D3DRTYPE_SURFACE,
											D3DPresentParams.AutoDepthStencilFormat);
	if (FAILED(hr))
	{
		Sys::Error("Rendering device doesn't support D24S8 depth buffer!\n");
		return;
	}

	// Check that the depth buffer format is compatible with the backbuffer format
	//???does support D3DFMT_UNKNOWN? if not, need to explicitly determine adapter (and also backbuffer) format.
	hr = pD3D9->CheckDepthStencilMatch(	Adapter,
										DEM_D3D_DEVICETYPE,
										D3DPresentParams.BackBufferFormat,
										D3DPresentParams.BackBufferFormat,
										D3DPresentParams.AutoDepthStencilFormat);
	if (FAILED(hr))
	{
		Sys::Error("Backbuffer format is not compatible with D24S8 depth buffer!\n");
		return;
	}

	// Can setup W-buffer: D3DCaps.RasterCaps | D3DPRASTERCAPS_WBUFFER -> D3DZB_USEW

#if DEM_D3D_DEBUG
	D3DPresentParams.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	DWORD BhvFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#else
	DWORD BhvFlags = (D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
		D3DCREATE_HARDWARE_VERTEXPROCESSING :
		D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#endif

	// NB: May fail if can't create requested number of backbuffers
	HRESULT hr = pD3D9->CreateDevice(Adapter,
									 DEM_D3D_DEVICETYPE,
									 D3D9DrvFactory->GetFocusWindow()->GetHWND(),
									 BhvFlags,
									 &D3DPresentParams,
									 &pD3DDevice);

	if (FAILED(hr))
	{
		//!!!DX9! Sys::Error("Failed to create Direct3D device object: %s!\n", DXGetErrorString(hr));
		FAIL;
	}

	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, /*Wireframe ? D3DFILL_WIREFRAME :*/ D3DFILL_SOLID);

	//!!!only to decide whether to clear stencil! clear if current DS surface includes stencil bits
	//???!!!RT must not have embedded DS surface, it is a separate class!
	//so there is no need in this field!
	CurrDepthStencilFormat =
		D3DPresentParams.EnableAutoDepthStencil ?  D3DPresentParams.AutoDepthStencilFormat : D3DFMT_UNKNOWN;

	CSwapChain& SC = *SwapChains.Reserve(1);
	SC.Desc = Desc;
	SC.pSwapChain = NULL;
	SC.TargetWindow = pWnd;

	return 0;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay, const CDisplayMode* pMode)
{
	if (pDisplay && Adapter != pDisplay->GetAdapterID()) FAIL;

	if (!pDisplay)
	{
		// system will select from window
		// or for d3d get adapter display format directly
	}

	CDisplayMode CurrentMode;
	if (!pMode)
	{
		//if (!pDisplay->GetCurrentMode(CurrentMode)) FAIL;
		pMode = &CurrentMode;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC->GetDesc(), SC->GetWindow(), D3DPresentParams);

	D3DPresentParams.Windowed = FALSE;
	D3DPresentParams.BackBufferWidth = pMode->Width;
	D3DPresentParams.BackBufferHeight = pMode->Height;
	D3DPresentParams.BackBufferFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(pMode->PixelFormat);

	//!!!Reset()!
	//Display.Fullscreen = !Display.Fullscreen;
	//ResetDevice();
	//Display.ResetWindow();

	OK;
}
//---------------------------------------------------------------------

//!!!set optional window pos & size! if not set, restore from somewhere!
bool CD3D9GPUDriver::SwitchToWindowed(DWORD SwapChainID)
{
	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC->GetDesc(), SC->GetWindow(), D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = SC->GetWindow()->GetWidth();
	D3DPresentParams.BackBufferHeight = SC->GetWindow()->GetHeight();
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	//!!!Reset()!
	//Display.Fullscreen = !Display.Fullscreen;
	//ResetDevice();
	//Display.ResetWindow();
}
//---------------------------------------------------------------------

}
