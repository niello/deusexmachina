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

// If device exists, creates additional swap chain. If device not exist, creates a device with implicit swap chain.
DWORD CD3D9GPUDriver::CreateSwapChain(const CSwapChainDesc& Desc, Sys::COSWindow* pWindow)
{
	if (pD3DDevice)
	{
		// Here we can create an additional swap chain and return its index
		// Index is determined as the first free element in SwapChains array
		// If there are no free elements, a new one must be reserved until a maximum is reached.
		return ERR_MAX_SWAP_CHAIN_COUNT_EXCEEDED;
	}

#if DEM_D3D_USENVPERFHUD
	Adapter = D3D9DrvFactory->GetAdapterCount() - 1; // NVPerfHUD adapter //???is always the last, or read adapter info?
#endif

	n_assert(D3D9DrvFactory->AdapterExists(Adapter));

	IDirect3D9* pD3D9 = D3D9DrvFactory->GetDirect3D9();

	memset(&D3DCaps, 0, sizeof(D3DCaps));
	n_assert(SUCCEEDED(pD3D9->GetDeviceCaps(Adapter, DEM_D3D_DEVICETYPE, &D3DCaps)));

	Sys::COSWindow* pWnd = pWindow ? pWindow : D3D9DrvFactory->GetFocusWindow();
	n_assert(pWnd);

	// Zero means matching window or display size.
	// But if at least one of these values specified, we should adjst window size.
	// A child window is an exception, we don't want rederer to resize it,
	// so we force a backbuffer size to a child window size.
	UINT BBWidth = Desc.BackBufferWidth, BBHeight = Desc.BackBufferHeight;
	if (BBWidth > 0 || BBHeight > 0)
	{
		if (pWnd->IsChild())
		{
			BBWidth = pWnd->GetWidth();
			BBHeight = pWnd->GetHeight();
		}
		else
		{
			Data::CRect WindowRect = pWnd->GetRect();
			if (BBWidth > 0) WindowRect.W = BBWidth;
			else BBWidth = WindowRect.W;
			if (BBHeight > 0) WindowRect.H = BBHeight;
			else BBHeight = WindowRect.H;
			pWnd->SetRect(WindowRect);
		}
	}

	//???why it is better to create windowed swap chain and then turn it fullscreen? except that it uses less arguments on creation.
	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(Desc, pWnd, D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = BBWidth;
	D3DPresentParams.BackBufferHeight = BBHeight;
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
		return ERR_CREATION_ERROR;
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
		return ERR_CREATION_ERROR;
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
	hr = pD3D9->CreateDevice(Adapter,
							DEM_D3D_DEVICETYPE,
							D3D9DrvFactory->GetFocusWindow()->GetHWND(),
							BhvFlags,
							&D3DPresentParams,
							&pD3DDevice);

	if (FAILED(hr))
	{
		//!!!DX9! Sys::Error("Failed to create Direct3D device object: %s!\n", DXGetErrorString(hr));
		return ERR_CREATION_ERROR;
	}

	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, /*Wireframe ? D3DFILL_WIREFRAME :*/ D3DFILL_SOLID);

	//!!!only to decide whether to clear stencil! clear if current DS surface includes stencil bits
	//???!!!RT must not have embedded DS surface, it is a separate class!
	//so there is no need in this field!
	//CurrDepthStencilFormat =
	//	D3DPresentParams.EnableAutoDepthStencil ?  D3DPresentParams.AutoDepthStencilFormat : D3DFMT_UNKNOWN;

	CSwapChain& SC = *SwapChains.Reserve(1);
	SC.Desc = Desc;
	SC.pSwapChain = NULL;
	SC.TargetWindow = pWnd;
	SC.LastWindowRect = pWnd->GetRect();

	return 0;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay, const CDisplayMode* pMode)
{
	if (SwapChainID >= (DWORD)SwapChains.GetCount()) FAIL;
	if (pDisplay && Adapter != pDisplay->GetAdapterID()) FAIL;

	CSwapChain& SC = SwapChains[SwapChainID];

	if (!pDisplay)
	{
		Sys::Error("CD3D9GPUDriver::SwitchToFullscreen > IMPLEMENT ME for pDisplay == NULL!!!");
		// system will select from window
		// or for d3d get adapter display format directly
	}

	CDisplayMode CurrentMode;
	if (!pMode)
	{
		if (!pDisplay->GetCurrentDisplayMode(CurrentMode)) FAIL;
		pMode = &CurrentMode;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC.Desc, SC.TargetWindow, D3DPresentParams);

	D3DPresentParams.Windowed = FALSE;
	D3DPresentParams.BackBufferWidth = pMode->Width;
	D3DPresentParams.BackBufferHeight = pMode->Height;
	D3DPresentParams.BackBufferFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(pMode->PixelFormat);

	SC.LastWindowRect = SC.TargetWindow->GetRect();
	SC.TargetWindow->SetRect(Data::CRect(0, 0, pMode->Width, pMode->Height), true);

	//ResetDevice();
	//or reset swap chain

	SC.pTargetDisplay = pDisplay;

	OK;
}
//---------------------------------------------------------------------

//!!!set optional window pos & size! if not set, restore from somewhere!
bool CD3D9GPUDriver::SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect)
{
	if (SwapChainID >= (DWORD)SwapChains.GetCount()) FAIL;

	CSwapChain& SC = SwapChains[SwapChainID];

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC.Desc, SC.TargetWindow, D3DPresentParams);

	const Data::CRect* pNewRect = pWindowRect ? pWindowRect : &SC.LastWindowRect;
	//!!!fill empty fields in passed rect with fields from a last rect!
	//!!!reset last rect, if explicit rect passed!

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = pNewRect->W;
	D3DPresentParams.BackBufferHeight = pNewRect->H;
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	//ResetDevice();
	//Display.ResetWindow();

	SC.TargetWindow->SetRect(*pNewRect);

	SC.pTargetDisplay = NULL;

	OK;
}
//---------------------------------------------------------------------

}

/*

void COSWindowWin32::CalcWindowRect(CRect& Rect)
{
	if (pParent)
	{
		Rect.X = 0;
		Rect.Y = 0;

		RECT r;
		::GetClientRect(pParent->GetHWND(), &r);
		::AdjustWindowRect(&r, STYLE_CHILD, FALSE);
		Rect.W = r.right - r.left; //???clamp w & h to parent rect?
		Rect.H = r.bottom - r.top;
	}
	else
	{
		CMonitorInfo MonitorInfo;
		GetAdapterMonitorInfo(Adapter, MonitorInfo);

		if (IsFullscreen)
		{
			if (DisplayModeSwitchEnabled)
			{
				X = MonitorInfo.Left;
				Y = MonitorInfo.Top;
			}
			else
			{
				X = MonitorInfo.Left + ((MonitorInfo.Width - DisplayMode.Width) / 2);
				Y = MonitorInfo.Top + ((MonitorInfo.Height - DisplayMode.Height) / 2);
			}
			W = DisplayMode.Width;
			H = DisplayMode.Height;
		}
		else
		{
			X = MonitorInfo.Left + DisplayMode.PosX;
			Y = MonitorInfo.Top + DisplayMode.PosY;
			RECT r = { X, Y, X + DisplayMode.Width, Y + DisplayMode.Height };
			::AdjustWindowRect(&r, STYLE_WINDOWED, FALSE);
			W = r.right - r.left;
			H = r.bottom - r.top;
		}
	}
}
//---------------------------------------------------------------------

//void COSWindowWin32::AdjustSize()
//{
//	n_assert(hWnd);
//
//	RECT r;
//	GetClientRect(hWnd, &r);
//	DisplayMode.Width = (ushort)r.right;
//	DisplayMode.Height = (ushort)r.bottom;
//
//	EventSrv->FireEvent(CStrID("OnDisplaySizeChanged"));
//}
////---------------------------------------------------------------------
*/