#include "D3D9GPUDriver.h"

#include <Render/D3D9/D3D9DriverFactory.h>
#include <Render/D3D9/D3D9DisplayDriver.h>
#include <Render/D3D9/D3D9VertexLayout.h>
#include <Render/D3D9/D3D9VertexBuffer.h>
#include <Render/D3D9/D3D9IndexBuffer.h>
#include <Render/D3D9/D3D9Texture.h>
#include <Render/D3D9/D3D9RenderTarget.h>
#include <Render/D3D9/D3D9DepthStencilBuffer.h>
#include <Render/D3D9/D3D9RenderState.h>
#include <Render/ImageUtils.h>
#include <Events/EventServer.h>
#include <IO/Stream.h>
#include <System/OSWindow.h>
#include <Core/Factory.h>

namespace Render
{
__ImplementClass(Render::CD3D9GPUDriver, 'D9GD', Render::CGPUDriver);

bool CD3D9GPUDriver::Init(DWORD AdapterNumber, EGPUDriverType DriverType)
{
	if (!CGPUDriver::Init(AdapterNumber, DriverType)) FAIL;

	n_assert(AdapterID != Adapter_AutoSelect || Type != GPU_AutoSelect);
	n_assert(AdapterID == Adapter_AutoSelect || D3D9DrvFactory->AdapterExists(AdapterID));

	Type = DriverType; // Cache requested value for further initialization
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::InitSwapChainRenderTarget(CD3D9SwapChain& SC)
{
	// Already initialized, can't reinitialize
	if (SC.BackBufferRT.IsValidPtr() && SC.BackBufferRT->IsValid()) FAIL;

	IDirect3DSurface9* pRTSurface = NULL;
	if (SC.pSwapChain)
	{
		if (FAILED(SC.pSwapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &pRTSurface))) FAIL;
	}
	else
	{
		if (FAILED(pD3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRTSurface))) FAIL;
	}

	if (!SC.BackBufferRT.IsValidPtr()) SC.BackBufferRT = n_new(CD3D9RenderTarget);
	if (!SC.BackBufferRT->As<CD3D9RenderTarget>()->Create(pRTSurface, NULL))
	{
		pRTSurface->Release();
		FAIL;
	}

	// Default swap chain may be automatically set as a render target
	if (!SC.pSwapChain)
	{
		IDirect3DSurface9* pCurrRTSurface = NULL;
		if (SUCCEEDED(pD3DDevice->GetRenderTarget(0, &pCurrRTSurface)) && pCurrRTSurface == pRTSurface)
			CurrRT[0] = (CD3D9RenderTarget*)(SC.BackBufferRT.GetUnsafe());
		if (pCurrRTSurface) pCurrRTSurface->Release();
	}

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::Reset(D3DPRESENT_PARAMETERS& D3DPresentParams, DWORD TargetSwapChainID)
{
	if (!pD3DDevice) FAIL;

	UNSUBSCRIBE_EVENT(OnPaint);

	//!!!unbind and release all resources!

	//for (int i = 0; i < VertexLayouts.GetCount() ; ++i)
	//{
	//	CVertexLayout* pVL = VertexLayouts.ValueAt(i).GetUnsafe();
	//	if (pVL) pVL->Destroy();
	//}
	//VertexLayouts.Clear();

	for (DWORD i = 0; i < CurrRT.GetCount() ; ++i)
		if (CurrRT[i].IsValidPtr())
		{
			CurrRT[i]->Destroy();
			CurrRT[i] = NULL;
		}
	
	if (CurrDS.IsValidPtr())
	{
		CurrDS->Destroy();
		CurrDS = NULL;
	}

	for (int i = 0; i < SwapChains.GetCount(); ++i)
		SwapChains[i].Release();

	//!!!ReleaseQueries();

	//???call some Release() code instead? kill all except the device itself!
	EventSrv->FireEvent(CStrID("OnRenderDeviceLost"));

	HRESULT hr = pD3DDevice->TestCooperativeLevel();
	while (hr != S_OK && hr != D3DERR_DEVICENOTRESET)
	{
		// NB: In single-threaded app, engine will stuck here until device can be reset
		//???instead of Sleep() return and allow application to do frame & msg processing? then start from here again.
		Sys::Sleep(10);
		hr = pD3DDevice->TestCooperativeLevel();
	}

	// When we make fullscreen -> windowed transition, we must set implicit params
	// from swap chain 0, but params passed may belong to another swap chain that was
	// fullscreen. In that case we apply params passed later in CreateAdditionalSwapChain().
	D3DPRESENT_PARAMETERS SwapChainD3DPresentParams;
	D3DPRESENT_PARAMETERS* pD3DParams;
	if (TargetSwapChainID == 0) pD3DParams = &D3DPresentParams;
	else
	{
		if (!GetCurrD3DPresentParams(SwapChains[0], SwapChainD3DPresentParams)) FAIL;
		pD3DParams = &SwapChainD3DPresentParams;
	}

	hr = pD3DDevice->Reset(pD3DParams);
	if (FAILED(hr))
	{
		Sys::Log("Failed to reset Direct3D9 device object!\n");
		FAIL;
	}

	// Can setup W-buffer: D3DCaps.RasterCaps | D3DPRASTERCAPS_WBUFFER -> D3DZB_USEW

	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, /*Wireframe ? D3DFILL_WIREFRAME :*/ D3DFILL_SOLID);

	bool IsFullscreenNow = (D3DPresentParams.Windowed == FALSE);
	bool Failed = false;

	for (int i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D9SwapChain& SC = SwapChains[i];

		if (!SC.IsValid()) continue;
		
		if (IsFullscreenNow)
		{
			// In a fullscreen mode only the fullscreen swap chain will be valid
			if (!SC.IsFullscreen()) continue;
			SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnPaint"), this, &CD3D9GPUDriver::OnOSWindowPaint, &Sub_OnPaint);
		}
		else
		{
			// Recreate additional swap chains. Skip implicit swap chain, index is always 0.
			if (i != 0)
			{
				if (TargetSwapChainID == i) pD3DParams = &D3DPresentParams;
				else
				{
					if (!GetCurrD3DPresentParams(SC, SwapChainD3DPresentParams))
					{
						n_assert_dbg(false);
						DestroySwapChain(i);
						Failed = true;
						continue;
					}
					pD3DParams = &SwapChainD3DPresentParams;
				}

				if (FAILED(pD3DDevice->CreateAdditionalSwapChain(pD3DParams, &SC.pSwapChain)))
				{
					n_assert_dbg(false);
					DestroySwapChain(i);
					Failed = true;
					continue;
				}
			}

			if (SC.Desc.Flags.Is(SwapChain_AutoAdjustSize))
				SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnSizeChanged"), this, &CD3D9GPUDriver::OnOSWindowSizeChanged, &SC.Sub_OnSizeChanged);
		}

		n_assert(InitSwapChainRenderTarget(SC));

		SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnToggleFullscreen"), this, &CD3D9GPUDriver::OnOSWindowToggleFullscreen, &SC.Sub_OnToggleFullscreen);
		SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnClosing"), this, &CD3D9GPUDriver::OnOSWindowClosing, &SC.Sub_OnClosing);

		if (IsFullscreenNow) break;
	}

	EventSrv->FireEvent(CStrID("OnRenderDeviceReset"));

	IsInsideFrame = false;

	//???!!!resize DS!? (if auto-resize enabled or DS is attached to an SC)

	if (Failed)
	{
		// Partially failed, can handle specifically
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::Release()
{
	if (!pD3DDevice) return;

	UNSUBSCRIBE_EVENT(OnPaint);

	//!!!UnbindD3D9Resources();
	//!!!can call the same event as on lost device!

	VertexLayouts.Clear();
	CurrRT.SetSize(0);

	//!!!if code won't be reused in Reset(), call DestroySwapChain()!
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].IsValid()) SwapChains[i].Destroy();

	//for (int i = 1; i < MaxRenderTargetCount; i++)
	//	pD3DDevice->SetRenderTarget(i, NULL);
	pD3DDevice->SetDepthStencilSurface(NULL);

	//EventSrv->FireEvent(CStrID("OnRenderDeviceRelease"));

	//!!!ReleaseQueries();

	pD3DDevice->Release();
	pD3DDevice = NULL;

	IsInsideFrame = false;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::CheckCaps(ECaps Cap)
{
	n_assert(pD3DDevice);

	switch (Cap)
	{
		case Caps_VSTexFiltering_Linear:
			return (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) && (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR);
		case Caps_VSTex_L16:
			return SUCCEEDED(D3D9DrvFactory->GetDirect3D9()->CheckDeviceFormat(	AdapterID,
																				GetD3DDriverType(Type),
																				D3DFMT_UNKNOWN, //D3DPresentParams.BackBufferFormat,
																				D3DUSAGE_QUERY_VERTEXTEXTURE,
																				D3DRTYPE_TEXTURE,
																				D3DFMT_L16));
		case Caps_ReadDepthAsTexture:
			return SUCCEEDED(D3D9DrvFactory->GetDirect3D9()->CheckDeviceFormat(	AdapterID,
																				GetD3DDriverType(Type),
																				D3DFMT_UNKNOWN, //D3DPresentParams.BackBufferFormat,
																				D3DUSAGE_DEPTHSTENCIL,
																				D3DRTYPE_SURFACE,
																				(D3DFORMAT)MAKEFOURCC('I','N','T','Z')));
		default: FAIL;
	}
}
//---------------------------------------------------------------------

DWORD CD3D9GPUDriver::GetMaxVertexStreams()
{
	n_assert(pD3DDevice);
	return D3DCaps.MaxStreams;
}
//---------------------------------------------------------------------

DWORD CD3D9GPUDriver::GetMaxTextureSize(ETextureType Type)
{
	n_assert(pD3DDevice);
	switch (Type)
	{
		case Texture_1D: return D3DCaps.MaxTextureWidth;
		case Texture_3D: return D3DCaps.MaxVolumeExtent;
		default: return n_min(D3DCaps.MaxTextureWidth, D3DCaps.MaxTextureHeight);
	}
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::FillD3DPresentParams(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc,
										  const Sys::COSWindow* pWindow, D3DPRESENT_PARAMETERS& D3DPresentParams) const
{
	D3DPresentParams.Flags = 0;

	DWORD BackBufferCount = SwapChainDesc.BackBufferCount ? SwapChainDesc.BackBufferCount : 1;

	switch (SwapChainDesc.SwapMode)
	{
		case SwapMode_CopyPersist:	D3DPresentParams.SwapEffect = BackBufferCount > 1 ? D3DSWAPEFFECT_FLIP : D3DSWAPEFFECT_COPY; break;
		//case SwapMode_FlipPersist:	// D3DSWAPEFFECT_FLIPEX, but it is available only in D3D9Ex
		case SwapMode_CopyDiscard:
		default:					D3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD; break; // Allows runtime to select the best
	}

	if (D3DPresentParams.SwapEffect == D3DSWAPEFFECT_DISCARD)
	{
		// NB: It is recommended to use non-MSAA swap chain, render to MSAA RT and then resolve it to the back buffer
		GetD3DMSAAParams(BackBufferDesc.MSAAQuality, CD3D9DriverFactory::PixelFormatToD3DFormat(BackBufferDesc.Format),
						 D3DPresentParams.MultiSampleType, D3DPresentParams.MultiSampleQuality);
	}
	else
	{
		// MSAA not supported for swap effects other than D3DSWAPEFFECT_DISCARD
		D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
		D3DPresentParams.MultiSampleQuality = 0;
	}

	D3DPresentParams.hDeviceWindow = pWindow->GetHWND();
	D3DPresentParams.BackBufferCount = BackBufferCount; //!!!N3 always sets 1 in windowed mode! why?
	D3DPresentParams.EnableAutoDepthStencil = FALSE;
	D3DPresentParams.AutoDepthStencilFormat = D3DFMT_UNKNOWN;

	// D3DPRESENT_INTERVAL_ONE - as _DEFAULT, but improves VSync quality at a little cost of processing time (uses another timer)
	D3DPresentParams.PresentationInterval = SwapChainDesc.Flags.Is(SwapChain_VSync) ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	D3DPresentParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;

#if !defined(_DEBUG) && (!defined(DEM_RENDER_DEBUG) || DEM_RENDER_DEBUG == 0)
	D3DPresentParams.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
#endif

#if defined(_DEBUG) && DEM_RENDER_DEBUG
	D3DPresentParams.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
#endif
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::GetCurrD3DPresentParams(const CD3D9SwapChain& SC, D3DPRESENT_PARAMETERS& D3DPresentParams) const
{
	FillD3DPresentParams(SC.BackBufferRT->GetDesc(), SC.Desc, SC.TargetWindow, D3DPresentParams);

	if (SC.IsFullscreen())
	{
		//!!!Can also request pDevice / pSC ->GetDisplayMode D3D9 method!
		CDisplayMode Mode;
		if (!SC.TargetDisplay->GetCurrentDisplayMode(Mode)) FAIL;
		D3DPresentParams.Windowed = FALSE;
		D3DPresentParams.BackBufferWidth = Mode.Width;
		D3DPresentParams.BackBufferHeight = Mode.Height;
		D3DPresentParams.BackBufferFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Mode.PixelFormat);
		//D3DPresentParams.FullScreen_RefreshRateInHz = Mode.RefreshRate.GetIntRounded();
	}
	else
	{
		const CRenderTargetDesc& BBDesc = SC.BackBufferRT->GetDesc();
		D3DPresentParams.Windowed = TRUE;
		D3DPresentParams.BackBufferWidth = BBDesc.Width;
		D3DPresentParams.BackBufferHeight = BBDesc.Height;
		D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode
	}

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::CreateD3DDevice(DWORD CurrAdapterID, EGPUDriverType CurrDriverType, D3DPRESENT_PARAMETERS D3DPresentParams)
{
	IDirect3D9* pD3D9 = D3D9DrvFactory->GetDirect3D9();

	D3DDEVTYPE D3DDriverType = GetD3DDriverType(CurrDriverType);

	memset(&D3DCaps, 0, sizeof(D3DCaps));
	n_assert(SUCCEEDED(pD3D9->GetDeviceCaps(CurrAdapterID, D3DDriverType, &D3DCaps)));

#if DEM_RENDER_DEBUG
	DWORD BhvFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#else
	DWORD BhvFlags = (D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
		D3DCREATE_HARDWARE_VERTEXPROCESSING :
		D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#endif

	// NB: May fail if can't create requested number of backbuffers
	HRESULT hr = pD3D9->CreateDevice(CurrAdapterID,
									D3DDriverType,
									D3D9DrvFactory->GetFocusWindow()->GetHWND(),
									BhvFlags,
									&D3DPresentParams,
									&pD3DDevice);

	if (FAILED(hr))
	{
		Sys::Log("Failed to create Direct3D9 device object, hr = 0x%x!\n", hr);
		FAIL;
	}

	CurrRT.SetSize(D3DCaps.NumSimultaneousRTs);

	OK;
}
//---------------------------------------------------------------------

// If device exists, creates additional swap chain. If device does not exist, creates a device with an implicit swap chain.
int CD3D9GPUDriver::CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow)
{
	n_assert2(!BackBufferDesc.UseAsShaderInput, "D3D9 backbuffer reading in shaders currently not supported!");

	Sys::COSWindow* pWnd = pWindow ? pWindow : D3D9DrvFactory->GetFocusWindow();
	n_assert(pWnd);

	//???or destroy and recreate with new params?
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd) return ERR_CREATION_ERROR;

	UINT BBWidth = BackBufferDesc.Width, BBHeight = BackBufferDesc.Height;
	PrepareWindowAndBackBufferSize(*pWnd, BBWidth, BBHeight);

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(BackBufferDesc, SwapChainDesc, pWnd, D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = BBWidth;
	D3DPresentParams.BackBufferHeight = BBHeight;
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	CArray<CD3D9SwapChain>::CIterator ItSC = NULL;

	if (pD3DDevice)
	{
		// As device exists, implicit swap chain exists too, so create additional one.
		// NB: additional swap chains work only in windowed mode

		IDirect3DSwapChain9* pD3DSwapChain = NULL;
		HRESULT hr = pD3DDevice->CreateAdditionalSwapChain(&D3DPresentParams, &pD3DSwapChain);

		if (FAILED(hr))
		{
			Sys::Error("Failed to create additional Direct3D9 swap chain!\n");
			return ERR_CREATION_ERROR;
		}

		for (ItSC = SwapChains.Begin(); ItSC != SwapChains.End(); ++ItSC)
			if (!ItSC->IsValid()) break;

		if (ItSC == SwapChains.End()) ItSC = SwapChains.Reserve(1);
	
		ItSC->pSwapChain = pD3DSwapChain;
	}
	else
	{
		// There is no swap chain nor device, create D3D9 device with an implicit swap chain

		n_assert(!SwapChains.GetCount());

		bool DeviceCreated = false;
		if (AdapterID == Adapter_AutoSelect)
		{
			DWORD AdapterCount = D3D9DrvFactory->GetAdapterCount();
			for (DWORD CurrAdapterID = 0; CurrAdapterID < AdapterCount; ++CurrAdapterID)
			{
				DeviceCreated = CreateD3DDevice(CurrAdapterID, Type, D3DPresentParams);
				if (DeviceCreated)
				{
					AdapterID = CurrAdapterID;
					break;
				}
			}
		}
		else if (Type == GPU_AutoSelect)
		{
			DeviceCreated = CreateD3DDevice(AdapterID, GPU_Hardware, D3DPresentParams);
			if (DeviceCreated) Type = GPU_Hardware;
			else
			{
				DeviceCreated = CreateD3DDevice(AdapterID, GPU_Reference, D3DPresentParams);
				if (DeviceCreated) Type = GPU_Reference;
			}
		}
		else DeviceCreated = CreateD3DDevice(AdapterID, Type, D3DPresentParams);

		if (!DeviceCreated) return ERR_CREATION_ERROR;

		// Can setup W-buffer: D3DCaps.RasterCaps | D3DPRASTERCAPS_WBUFFER -> D3DZB_USEW

		pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
		pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
		pD3DDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

		ItSC = SwapChains.IteratorAt(0); 
		ItSC->pSwapChain = NULL;
	}

	if (!InitSwapChainRenderTarget(*ItSC))
	{
		ItSC->Release();
		return ERR_CREATION_ERROR;
	}

	ItSC->TargetWindow = pWnd;
	ItSC->LastWindowRect = pWnd->GetRect();
	ItSC->TargetDisplay = NULL;
	ItSC->Desc = SwapChainDesc;

	pWnd->Subscribe<CD3D9GPUDriver>(CStrID("OnToggleFullscreen"), this, &CD3D9GPUDriver::OnOSWindowToggleFullscreen, &ItSC->Sub_OnToggleFullscreen);
	pWnd->Subscribe<CD3D9GPUDriver>(CStrID("OnClosing"), this, &CD3D9GPUDriver::OnOSWindowClosing, &ItSC->Sub_OnClosing);
	if (SwapChainDesc.Flags.Is(SwapChain_AutoAdjustSize))
		pWnd->Subscribe<CD3D9GPUDriver>(CStrID("OnSizeChanged"), this, &CD3D9GPUDriver::OnOSWindowSizeChanged, &ItSC->Sub_OnSizeChanged);

	return SwapChains.IndexOf(ItSC);
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::DestroySwapChain(DWORD SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	
	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	// Never unset 0'th RT
	for (DWORD i = 1; i < CurrRT.GetCount(); ++i)
		if (CurrRT[i].GetUnsafe() == SC.BackBufferRT.GetUnsafe())
			SetRenderTarget(i, NULL);

	if (SwapChainID != 0 && SC.IsFullscreen()) SwitchToWindowed(SwapChainID);

	SC.Destroy();

	// Default swap chain destroyed means device is destroyed too
	if (SwapChainID == 0) Release();

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwapChainExists(DWORD SwapChainID) const
{
	return SwapChainID < (DWORD)SwapChains.GetCount() && SwapChains[SwapChainID].IsValid();
}
//---------------------------------------------------------------------

// Does not resize an OS window, since often is called in response to an OS window resize
//???bool ResizeOSWindow?
bool CD3D9GPUDriver::ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];
	const CRenderTargetDesc& BackBufDesc = SC.BackBufferRT->GetDesc();

	if ((!Width || BackBufDesc.Width == Width) && (!Height || BackBufDesc.Height == Height)) OK;

	//???for child window, assert that size passed is a window size?

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	if (!GetCurrD3DPresentParams(SC, D3DPresentParams)) FAIL;

	if (SC.IsFullscreen())
	{
		CDisplayMode Mode(
			Width ? Width : D3DPresentParams.BackBufferWidth,
			Height ? Height : D3DPresentParams.BackBufferHeight,
			CD3D9DriverFactory::D3DFormatToPixelFormat(D3DPresentParams.BackBufferFormat));
		if (!SC.TargetDisplay->SupportsDisplayMode(Mode))
		{
			//!!!if (!SC.pTargetDisplay->GetClosestDisplayMode(inout Mode)) FAIL;!
			FAIL;
		}
		D3DPresentParams.BackBufferWidth = Mode.Width;
		D3DPresentParams.BackBufferHeight = Mode.Height;
	}
	else
	{
		if (Width) D3DPresentParams.BackBufferWidth = Width;
		if (Height) D3DPresentParams.BackBufferHeight = Height;
	}

	if (SC.pSwapChain && !SC.IsFullscreen())
	{
		SC.BackBufferRT->Destroy();
		SC.pSwapChain->Release();
		if (FAILED(pD3DDevice->CreateAdditionalSwapChain(&D3DPresentParams, &SC.pSwapChain))) FAIL;
		n_assert(InitSwapChainRenderTarget(SC));
	}
	else if (!Reset(D3DPresentParams, SwapChainID)) FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwitchToFullscreen(DWORD SwapChainID, CDisplayDriver* pDisplay, const CDisplayMode* pMode)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	if (pDisplay && AdapterID != pDisplay->GetAdapterID()) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	if (SC.TargetWindow->IsChild())
	{
		n_assert2(false, "WORTH TESTING!");
		FAIL;
	}

	// Only one output per adapter in a current implementation, use output ID 0 for default display
	SC.TargetDisplay = pDisplay ? pDisplay : D3D9DrvFactory->CreateDisplayDriver(AdapterID, 0);
	if (SC.TargetDisplay.IsNullPtr()) FAIL;

	CDisplayMode CurrentMode;
	if (!pMode)
	{
		if (!SC.TargetDisplay->GetCurrentDisplayMode(CurrentMode))
		{
			SC.TargetDisplay = NULL;
			FAIL;
		}
		pMode = &CurrentMode;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC.BackBufferRT->GetDesc(), SC.Desc, SC.TargetWindow, D3DPresentParams);

	D3DPresentParams.Windowed = FALSE;
	D3DPresentParams.BackBufferWidth = pMode->Width;
	D3DPresentParams.BackBufferHeight = pMode->Height;
	D3DPresentParams.BackBufferFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(pMode->PixelFormat);
	//D3DPresentParams.FullScreen_RefreshRateInHz = pMode->RefreshRate.GetIntRounded();

	// Resize window without resizing a swap chain
	CDisplayDriver::CMonitorInfo MonInfo;
	if (!SC.TargetDisplay->GetDisplayMonitorInfo(MonInfo))
	{
		SC.TargetDisplay = NULL;
		FAIL;
	}
	SC.Sub_OnSizeChanged = NULL;
	SC.LastWindowRect = SC.TargetWindow->GetRect();
	SC.TargetWindow->SetRect(Data::CRect(MonInfo.Left, MonInfo.Top, pMode->Width, pMode->Height), true);
	//!!!check what system does with window on fullscreen transition! mb it resizes wnd itself.

	if (!Reset(D3DPresentParams, 0)) // Fullscreen swap chain may only be implicit
	{
		//???call SwitchToWindowed?
		SC.TargetWindow->SetRect(SC.LastWindowRect);
		if (SC.Desc.Flags.Is(SwapChain_AutoAdjustSize))
			SC.TargetWindow->Subscribe<CD3D9GPUDriver>(CStrID("OnSizeChanged"), this, &CD3D9GPUDriver::OnOSWindowSizeChanged, &SC.Sub_OnSizeChanged);
		FAIL;
	}

	SC.TargetWindow->SetInputFocus();

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	if (pWindowRect)
	{
		SC.LastWindowRect.X = pWindowRect->X;
		SC.LastWindowRect.Y = pWindowRect->Y;
		if (pWindowRect->W > 0) SC.LastWindowRect.W = pWindowRect->W;
		if (pWindowRect->H > 0) SC.LastWindowRect.H = pWindowRect->H;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC.BackBufferRT->GetDesc(), SC.Desc, SC.TargetWindow, D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = SC.LastWindowRect.W;
	D3DPresentParams.BackBufferHeight = SC.LastWindowRect.H;
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	SC.TargetDisplay = NULL;
	SC.Sub_OnSizeChanged = NULL;
	SC.TargetWindow->SetRect(SC.LastWindowRect);

	return Reset(D3DPresentParams, SwapChainID);
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::IsFullscreen(DWORD SwapChainID) const
{
	return SwapChainExists(SwapChainID) && SwapChains[SwapChainID].IsFullscreen();
}
//---------------------------------------------------------------------

PRenderTarget CD3D9GPUDriver::GetSwapChainRenderTarget(DWORD SwapChainID) const
{
	return SwapChainExists(SwapChainID) ? SwapChains[SwapChainID].BackBufferRT : PRenderTarget();
}
//---------------------------------------------------------------------

// NB: Present() should be called as late as possible after EndFrame()
// to improve parallelism between the GPU and the CPU
bool CD3D9GPUDriver::Present(DWORD SwapChainID)
{
	if (IsInsideFrame || !SwapChainExists(SwapChainID)) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];

	// For swap chain: Present will fail if called between BeginScene and EndScene pairs unless the
	// render target is not the current render target. //???so don't fail if IsInsideFrame?
	HRESULT hr = SC.pSwapChain ? SC.pSwapChain->Present(NULL, NULL, NULL, NULL, 0) : pD3DDevice->Present(NULL, NULL, NULL, NULL);
	if (FAILED(hr))
	{
		if (hr == D3DERR_DEVICELOST)
		{
			CD3D9SwapChain& ImplicitSC = SwapChains[0];

			D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
			if (!GetCurrD3DPresentParams(ImplicitSC, D3DPresentParams)) FAIL;
			if (!Reset(D3DPresentParams, 0)) FAIL;
		}
		else if (hr == D3DERR_INVALIDCALL) FAIL;
		else // D3DERR_DRIVERINTERNALERROR, D3DERR_OUTOFVIDEOMEMORY, E_OUTOFMEMORY
		{
			// Destroy and recreate device and all swap chains
			Sys::Error("CD3D9GPUDriver::Present() > IMPLEMENT ME FAILED(hr)!!!");
			FAIL;
		}
	}
	else ++SC.FrameID;

	//// Sync CPU thread with GPU
	//// wait till gpu has finsihed rendering the previous frame
	//gpuSyncQuery[frameId % numSyncQueries]->Issue(D3DISSUE_END);                              
	//++FrameID; //???why here?
	//while (S_FALSE == gpuSyncQuery[frameId % numSyncQueries]->GetData(NULL, 0, D3DGETDATA_FLUSH)) ;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::WriteScreenshot(DWORD SwapChainID, IO::CStream& OutStream) const
{
	if (!pD3DDevice || IsInsideFrame || !SwapChainExists(SwapChainID)) FAIL;

	CD3D9SwapChain& SC = SwapChains[SwapChainID];
	const CD3D9RenderTarget* pRT = (const CD3D9RenderTarget*)SC.BackBufferRT.GetUnsafe();
	if (!pRT) FAIL;

	const CRenderTargetDesc& Desc = pRT->GetDesc();

	IDirect3DSurface9* pCaptureSurface = NULL;
	HRESULT hr = pD3DDevice->CreateOffscreenPlainSurface(	Desc.Width,
															Desc.Height,
															CD3D9DriverFactory::PixelFormatToD3DFormat(Desc.Format),
															D3DPOOL_SYSTEMMEM,
															&pCaptureSurface,
															NULL);
	if (FAILED(hr) || !pCaptureSurface) FAIL;

	if (FAILED(pD3DDevice->GetRenderTargetData(pRT->GetD3DSurface(), pCaptureSurface))) FAIL;

	D3DLOCKED_RECT D3DRect;
	if (FAILED(pCaptureSurface->LockRect(&D3DRect, NULL, D3DLOCK_READONLY)))
	{
		pCaptureSurface->Release();
		FAIL;
	}

	bool WasOpen = OutStream.IsOpen();
	if (WasOpen || OutStream.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL))
	{
		OutStream.Write(D3DRect.pBits, D3DRect.Pitch * Desc.Height);
		if (!WasOpen) OutStream.Close();
	}

	const bool Result = SUCCEEDED(pCaptureSurface->UnlockRect());
	pCaptureSurface->Release();
	return Result;
}
//---------------------------------------------------------------------

//!!!if cache VP, handle implicit cjanges like on set RT!

bool CD3D9GPUDriver::SetViewport(DWORD Index, const CViewport* pViewport)
{
	if (!pD3DDevice || Index > 0) FAIL;

	//???store curr VP not to reset?

	D3DVIEWPORT9* pD3DVP = NULL;
	if (pViewport)
	{
		D3DVIEWPORT9 D3DVP;
		D3DVP.X = (DWORD)pViewport->Left;
		D3DVP.Y = (DWORD)pViewport->Top;
		D3DVP.Width = (DWORD)pViewport->Width;
		D3DVP.Height = (DWORD)pViewport->Height;
		D3DVP.MinZ = pViewport->MinDepth;
		D3DVP.MaxZ = pViewport->MaxDepth;
		pD3DVP = &D3DVP;
	}

	return SUCCEEDED(pD3DDevice->SetViewport(pD3DVP));
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::GetViewport(DWORD Index, CViewport& OutViewport)
{
	if (!pD3DDevice || Index > 0) FAIL;

	D3DVIEWPORT9 D3DVP = { 0 };
	if (FAILED(pD3DDevice->GetViewport(&D3DVP))) FAIL;

	OutViewport.Left = (float)D3DVP.X;
	OutViewport.Top = (float)D3DVP.Y;
	OutViewport.Width = (float)D3DVP.Width;
	OutViewport.Height = (float)D3DVP.Height;
	OutViewport.MinDepth = D3DVP.MinZ;
	OutViewport.MaxDepth = D3DVP.MaxZ;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SetScissorRect(DWORD Index, const Data::CRect* pScissorRect)
{
	if (!pD3DDevice || Index > 0) FAIL;

	//???store curr SR not to reset?

	RECT* pSR = NULL;
	if (pScissorRect)
	{
		RECT SR;
		SR.left = pScissorRect->X;
		SR.top = pScissorRect->Y;
		SR.right = pScissorRect->Right();
		SR.bottom = pScissorRect->Bottom();
		pSR = &SR;
	}

	return SUCCEEDED(pD3DDevice->SetScissorRect(pSR));
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::GetScissorRect(DWORD Index, Data::CRect& OutScissorRect)
{
	if (!pD3DDevice || Index > 0) FAIL;

	RECT SR = { 0 };
	if (FAILED(pD3DDevice->GetScissorRect(&SR))) FAIL;

	OutScissorRect.X = SR.left;
	OutScissorRect.Y = SR.top;
	OutScissorRect.W = SR.right - SR.left;
	OutScissorRect.H = SR.bottom - SR.top;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::BeginFrame()
{
	return pD3DDevice && SUCCEEDED(pD3DDevice->BeginScene());

//	n_assert(!IsInsideFrame);
//
//	PrimsRendered = 0;
//	DIPsRendered = 0;
//
//	//???where? once per frame shader change
//	if (!SharedShader.IsValid())
//	{
//		SharedShader = ShaderMgr.GetTypedResource(CStrID("Shared"));
//		n_assert(SharedShader->IsLoaded());
//		hLightAmbient = SharedShader->GetVarHandleByName(CStrID("LightAmbient"));
//		hEyePos = SharedShader->GetVarHandleByName(CStrID("EyePos"));
//		hViewProj = SharedShader->GetVarHandleByName(CStrID("ViewProjection"));
//	}
//
//	// CEGUI overwrites this value without restoring it, so restore each frame
//	pD3DDevice->SetRenderState(D3DRS_FILLMODE, Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
//
//	IsInsideFrame = SUCCEEDED(pD3DDevice->BeginScene());
//	return IsInsideFrame;
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::EndFrame()
{
	pD3DDevice->EndScene();

//	n_assert(IsInsideFrame);
//	n_assert(SUCCEEDED(pD3DDevice->EndScene()));
//	IsInsideFrame = false;
//
//	//???is all below necessary? PIX requires it for debugging frame
//	for (int i = 0; i < MaxVertexStreamCount; ++i)
//		CurrVB[i] = NULL;
//	CurrVLayout = NULL;
//	CurrIB = NULL;
//	//!!!UnbindD3D9Resources()
//
//	CoreSrv->SetGlobal<int>("Render_Prim", PrimsRendered);
//	CoreSrv->SetGlobal<int>("Render_DIP", DIPsRendered);
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SetVertexLayout(CVertexLayout* pVLayout)
{
	if (CurrVL.GetUnsafe() == pVLayout) OK;
	IDirect3DVertexDeclaration9* pDecl = pVLayout ? ((CD3D9VertexLayout*)pVLayout)->GetD3DVertexDeclaration() : NULL;
	if (FAILED(pD3DDevice->SetVertexDeclaration(pDecl))) FAIL;
	CurrVL = (CD3D9VertexLayout*)pVLayout;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex)
{
	if (Index >= CurrVB.GetCount() || (pVB && OffsetVertex >= pVB->GetVertexCount())) FAIL;

	if (CurrVB[Index].GetUnsafe() == pVB && CurrVBOffset[Index] == OffsetVertex) OK;

	IDirect3DVertexBuffer9* pD3DVB = pVB ? ((CD3D9VertexBuffer*)pVB)->GetD3DBuffer() : NULL;
	DWORD VertexSize = pVB ? pVB->GetVertexLayout()->GetVertexSizeInBytes() : 0;
	if (FAILED(pD3DDevice->SetStreamSource(Index, pD3DVB, VertexSize * OffsetVertex, VertexSize))) FAIL;
	CurrVB[Index] = (CD3D9VertexBuffer*)pVB;
	CurrVBOffset[Index] = OffsetVertex;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SetIndexBuffer(CIndexBuffer* pIB)
{
	if (CurrIB.GetUnsafe() == pIB) OK;
	IDirect3DIndexBuffer9* pD3DIB = pIB ? ((CD3D9IndexBuffer*)pIB)->GetD3DBuffer() : NULL;
	if (FAILED(pD3DDevice->SetIndices(pD3DIB))) FAIL;
	CurrIB = (CD3D9IndexBuffer*)pIB;
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SetRenderTarget(DWORD Index, CRenderTarget* pRT)
{
	if (Index >= CurrRT.GetCount()) FAIL;
	if (CurrRT[Index].GetUnsafe() == pRT) OK;

	if (!pRT && Index == 0)
	{
		// Invalid case for D3D9. Restore main RT to default backbuffer.
		for (int i = 0; i < SwapChains.GetCount(); ++i)
		{
			CD3D9SwapChain& SC = SwapChains[i];
			if (SC.IsValid() && !SC.pSwapChain)
			{
				pRT = SC.BackBufferRT.GetUnsafe();
				break;
			}
		}
	}

	n_assert_dbg(pRT || Index > 0); // D3D9 can't set NULL to 0'th RT

	IDirect3DSurface9* pRTSurface = pRT ? ((CD3D9RenderTarget*)pRT)->GetD3DSurface() : NULL;
	if (FAILED(pD3DDevice->SetRenderTarget(Index, pRTSurface))) FAIL;
	CurrRT[Index] = (CD3D9RenderTarget*)pRT;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::SetDepthStencilBuffer(CDepthStencilBuffer* pDS)
{
	if (CurrDS.GetUnsafe() == pDS) OK;
	IDirect3DSurface9* pDSSurface = pDS ? ((CD3D9DepthStencilBuffer*)pDS)->GetD3DSurface() : NULL;
	if (FAILED(pD3DDevice->SetDepthStencilSurface(pDSSurface))) FAIL;
	CurrDS = (CD3D9DepthStencilBuffer*)pDS;
	OK;

	//!!!COMPATIBILITY TEST! (separate method)
	//!!!also test MSAA compatibility (all the same)!
	//// Check that the depth buffer format is compatible with the backbuffer format
	//hr = pD3D9->CheckDepthStencilMatch(	AdapterID,
	//									D3DDriverType,
	//									D3DPresentParams.BackBufferFormat,
	//									D3DPresentParams.BackBufferFormat,
	//									D3DPresentParams.AutoDepthStencilFormat);
	//if (FAILED(hr)) return NULL;
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::Clear(DWORD Flags, const vector4& ColorRGBA, float Depth, uchar Stencil)
{
	if (!Flags) return;

	DWORD D3DFlags = 0;

	if (Flags & Clear_Color) D3DFlags |= D3DCLEAR_TARGET;

	if (CurrDS.IsValidPtr())
	{
		if (Flags & Clear_Depth) D3DFlags |= D3DCLEAR_ZBUFFER;

		D3DFORMAT Fmt = CD3D9DriverFactory::PixelFormatToD3DFormat(CurrDS->GetDesc().Format);
		if ((Flags & Clear_Stencil) && CD3D9DriverFactory::D3DFormatStencilBits(Fmt) > 0)
			D3DFlags |= D3DCLEAR_STENCIL;
	}

	DWORD ColorARGB =
		(((uchar)(ColorRGBA.w * 255.f)) << 24) +
		(((uchar)(ColorRGBA.x * 255.f)) << 16) +
		(((uchar)(ColorRGBA.y * 255.f)) << 8) +
		((uchar)(ColorRGBA.z * 255.f));
	n_assert(SUCCEEDED(pD3DDevice->Clear(0, NULL, D3DFlags, ColorARGB, Depth, Stencil)));
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA)
{
	if (!RT.IsValid()) return;

	DWORD ColorARGB =
		(((uchar)(ColorRGBA.w * 255.f)) << 24) +
		(((uchar)(ColorRGBA.x * 255.f)) << 16) +
		(((uchar)(ColorRGBA.y * 255.f)) << 8) +
		((uchar)(ColorRGBA.z * 255.f));

	CD3D9RenderTarget& D3D9RT = (CD3D9RenderTarget&)RT;
	pD3DDevice->ColorFill(D3D9RT.GetD3DSurface(), NULL, ColorARGB);

	//pD3DDevice->SetRenderTarget(0, D3D9RT.GetD3DSurface());

	//for (DWORD i = 1; i < CurrRT.GetCount(); ++i)
	//	if (CurrRT[i].IsValidPtr() && CurrRT[i]->IsValid())
	//		pD3DDevice->SetRenderTarget(i, NULL);

	//Clear(Clear_Color, ColorRGBA, 1.f, 0);

	//for (DWORD i = 0; i < CurrRT.GetCount(); ++i)
	//	if (CurrRT[i].IsValidPtr() && CurrRT[i]->IsValid())
	//		pD3DDevice->SetRenderTarget(i, CurrRT[i]->GetD3DSurface());
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::Draw(const CPrimitiveGroup& PrimGroup)
{
	n_assert_dbg(pD3DDevice && IsInsideFrame);

	D3DPRIMITIVETYPE D3DPrimType;
	DWORD PrimCount = (PrimGroup.IndexCount > 0) ? PrimGroup.IndexCount : PrimGroup.VertexCount;
	switch (PrimGroup.Topology)
	{
		case Prim_PointList:	D3DPrimType = D3DPT_POINTLIST; break;
		case Prim_LineList:		D3DPrimType = D3DPT_LINELIST; PrimCount >>= 1; break;
		case Prim_LineStrip:	D3DPrimType = D3DPT_LINESTRIP; --PrimCount; break;
		case Prim_TriList:		D3DPrimType = D3DPT_TRIANGLELIST; PrimCount /= 3; break;
		case Prim_TriStrip:		D3DPrimType = D3DPT_TRIANGLESTRIP; PrimCount -= 2; break;
		default:				Sys::Error("CD3D9GPUDriver::Draw() -> Invalid primitive topology!"); FAIL;
	}

	HRESULT hr;
	if (PrimGroup.IndexCount > 0)
	{
		n_assert_dbg(CurrIB.IsValidPtr());
		//n_assert_dbg(!InstanceCount || CurrVB[0].IsValid());
		hr = pD3DDevice->DrawIndexedPrimitive(	D3DPrimType,
												0,
												PrimGroup.FirstVertex,
												PrimGroup.VertexCount,
												PrimGroup.FirstIndex,
												PrimCount);
	}
	else
	{
		//n_assert2_dbg(!InstanceCount, "Non-indexed instanced rendereng is not supported by design!");
		hr = pD3DDevice->DrawPrimitive(D3DPrimType, PrimGroup.FirstVertex, PrimCount);
	}

	//PrimsRendered += InstanceCount ? InstanceCount * PrimCount : PrimCount;
	//++DIPsRendered;

	return SUCCEEDED(hr);
}
//---------------------------------------------------------------------

PVertexLayout CD3D9GPUDriver::CreateVertexLayout(const CVertexComponent* pComponents, DWORD Count)
{
	const DWORD MAX_VERTEX_COMPONENTS = 32;

	if (!pComponents || !Count || Count > MAX_VERTEX_COMPONENTS) return NULL;

	CStrID Signature = CVertexLayout::BuildSignature(pComponents, Count);

	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx).GetUnsafe();

	D3DVERTEXELEMENT9 DeclData[MAX_VERTEX_COMPONENTS] = { 0 };

	DWORD* StreamOffset = (DWORD*)_malloca(D3DCaps.MaxStreams * sizeof(DWORD));
	ZeroMemory(StreamOffset, D3DCaps.MaxStreams * sizeof(DWORD));

	DWORD i = 0;
	for (i = 0; i < Count; i++)
	{
		const CVertexComponent& Component = pComponents[i];

		WORD StreamIndex = (WORD)Component.Stream;
		if (StreamIndex >= D3DCaps.MaxStreams)
		{
			_freea(StreamOffset);
			return NULL;
		}

		D3DVERTEXELEMENT9& DeclElement = DeclData[i];

		DeclElement.Stream = StreamIndex;

		DeclElement.Offset =
			(Component.OffsetInVertex == DEM_VERTEX_COMPONENT_OFFSET_DEFAULT) ? (WORD)StreamOffset[StreamIndex] : (WORD)Component.OffsetInVertex;
		StreamOffset[StreamIndex] = DeclElement.Offset + Component.GetSize();

		switch (Component.Format)
		{
			case VCFmt_Float32_1:		DeclElement.Type = D3DDECLTYPE_FLOAT1; break;
			case VCFmt_Float32_2:		DeclElement.Type = D3DDECLTYPE_FLOAT2; break;
			case VCFmt_Float32_3:		DeclElement.Type = D3DDECLTYPE_FLOAT3; break;
			case VCFmt_Float32_4:		DeclElement.Type = D3DDECLTYPE_FLOAT4; break;
			case VCFmt_Float16_2:		DeclElement.Type = D3DDECLTYPE_FLOAT16_2; break;
			case VCFmt_Float16_4:		DeclElement.Type = D3DDECLTYPE_FLOAT16_4; break;
			case VCFmt_UInt8_4:			DeclElement.Type = D3DDECLTYPE_UBYTE4; break;
			case VCFmt_UInt8_4_Norm:	DeclElement.Type = D3DDECLTYPE_UBYTE4N; break;
			case VCFmt_SInt16_2:		DeclElement.Type = D3DDECLTYPE_SHORT2; break;
			case VCFmt_SInt16_4:		DeclElement.Type = D3DDECLTYPE_SHORT4; break;
			case VCFmt_SInt16_2_Norm:	DeclElement.Type = D3DDECLTYPE_SHORT2N; break;
			case VCFmt_SInt16_4_Norm:	DeclElement.Type = D3DDECLTYPE_SHORT4N; break;
			case VCFmt_UInt16_2_Norm:	DeclElement.Type = D3DDECLTYPE_USHORT2N; break;
			case VCFmt_UInt16_4_Norm:	DeclElement.Type = D3DDECLTYPE_USHORT4N; break;
			default:
			{
				_freea(StreamOffset);
				return NULL;
			}
		}

		DeclElement.Method = D3DDECLMETHOD_DEFAULT;

		//???D3DDECLUSAGE_PSIZE?
		switch (Component.Semantic)
		{
			case VCSem_Position:	DeclElement.Usage = D3DDECLUSAGE_POSITION; break;
			case VCSem_Normal:		DeclElement.Usage = D3DDECLUSAGE_NORMAL; break;
			case VCSem_Tangent:		DeclElement.Usage = D3DDECLUSAGE_TANGENT; break;
			case VCSem_Bitangent:	DeclElement.Usage = D3DDECLUSAGE_BINORMAL; break;
			case VCSem_TexCoord:	DeclElement.Usage = D3DDECLUSAGE_TEXCOORD; break;
			case VCSem_Color:		DeclElement.Usage = D3DDECLUSAGE_COLOR; break;
			case VCSem_BoneWeights:	DeclElement.Usage = D3DDECLUSAGE_BLENDWEIGHT; break;
			case VCSem_BoneIndices:	DeclElement.Usage = D3DDECLUSAGE_BLENDINDICES; break;
			default:
			{
				_freea(StreamOffset);
				return NULL;
			}
		}

		DeclElement.UsageIndex = (BYTE)Component.Index;
	}

	_freea(StreamOffset);

	DeclData[i].Stream = 0xff;
	DeclData[i].Type = D3DDECLTYPE_UNUSED;

	IDirect3DVertexDeclaration9* pDecl = NULL;
	if (FAILED(pD3DDevice->CreateVertexDeclaration(DeclData, &pDecl))) return NULL;

	PD3D9VertexLayout Layout = n_new(CD3D9VertexLayout);
	if (!Layout->Create(pComponents, Count, pDecl))
	{
		pDecl->Release();
		return NULL;
	}

	VertexLayouts.Add(Signature, Layout);

	return Layout.GetUnsafe();
}
//---------------------------------------------------------------------

PVertexBuffer CD3D9GPUDriver::CreateVertexBuffer(CVertexLayout& VertexLayout, DWORD VertexCount, DWORD AccessFlags, const void* pData)
{
	if (!pD3DDevice || !VertexCount || !VertexLayout.GetVertexSizeInBytes()) return NULL;

	DWORD Usage;
	D3DPOOL Pool;
	GetUsagePool(AccessFlags, Usage, Pool);
	if (Pool == D3DPOOL_DEFAULT) Usage |= D3DUSAGE_WRITEONLY;

	DWORD ByteSize = VertexCount * VertexLayout.GetVertexSizeInBytes();

	IDirect3DVertexBuffer9* pD3DBuf = NULL;
	if (FAILED(pD3DDevice->CreateVertexBuffer(ByteSize, Usage, 0, Pool, &pD3DBuf, NULL))) return NULL;

	if (pData)
	{
		void* pDestData = NULL;
		if (FAILED(pD3DBuf->Lock(0, 0, &pDestData, D3DLOCK_NOSYSLOCK)))
		{
			pD3DBuf->Release();
			return NULL;
		}

		memcpy(pDestData, pData, ByteSize);

		if (FAILED(pD3DBuf->Unlock()))
		{
			pD3DBuf->Release();
			return NULL;
		}
	}

	PD3D9VertexBuffer VB = n_new(CD3D9VertexBuffer);
	if (!VB->Create(VertexLayout, pD3DBuf))
	{
		pD3DBuf->Release();
		return NULL;
	}

	return VB.GetUnsafe();
}
//---------------------------------------------------------------------

PIndexBuffer CD3D9GPUDriver::CreateIndexBuffer(EIndexType IndexType, DWORD IndexCount, DWORD AccessFlags, const void* pData)
{
	if (!pD3DDevice || !IndexCount) return NULL;

	DWORD Usage;
	D3DPOOL Pool;
	GetUsagePool(AccessFlags, Usage, Pool);
	if (Pool == D3DPOOL_DEFAULT) Usage |= D3DUSAGE_WRITEONLY;

	D3DFORMAT Format = (IndexType == Index_16) ? D3DFMT_INDEX16 : D3DFMT_INDEX32;
	DWORD ByteSize = IndexCount * (DWORD)IndexType;

	IDirect3DIndexBuffer9* pD3DBuf = NULL;
	if (FAILED(pD3DDevice->CreateIndexBuffer(ByteSize, Usage, Format, Pool, &pD3DBuf, NULL))) return NULL;

	if (pData)
	{
		void* pDestData = NULL;
		if (FAILED(pD3DBuf->Lock(0, 0, &pDestData, D3DLOCK_NOSYSLOCK)))
		{
			pD3DBuf->Release();
			return NULL;
		}

		memcpy(pDestData, pData, ByteSize);

		if (FAILED(pD3DBuf->Unlock()))
		{
			pD3DBuf->Release();
			return NULL;
		}
	}

	PD3D9IndexBuffer IB = n_new(CD3D9IndexBuffer);
	if (!IB->Create(IndexType, pD3DBuf))
	{
		pD3DBuf->Release();
		return NULL;
	}

	return IB.GetUnsafe();
}
//---------------------------------------------------------------------

//???how to handle MipDataProvided? lock levels one by one and upload data?
PTexture CD3D9GPUDriver::CreateTexture(const CTextureDesc& Desc, DWORD AccessFlags, const void* pData, bool MipDataProvided)
{
	if (!pD3DDevice) return NULL;

	PD3D9Texture Tex = n_new(CD3D9Texture);
	if (Tex.IsNullPtr()) return NULL;

	D3DFORMAT D3DFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Desc.Format);

	DWORD Usage;
	D3DPOOL Pool;
	GetUsagePool(AccessFlags, Usage, Pool);
	if (Desc.MipLevels != 1) Usage |= D3DUSAGE_AUTOGENMIPMAP;

	if (Desc.Type == Texture_1D || Desc.Type == Texture_2D)
	{
		UINT Height = (Desc.Type == Texture_1D) ? 1 : Desc.Height;
		IDirect3DTexture9* pD3DTex = NULL;
		if (FAILED(pD3DDevice->CreateTexture(Desc.Width, Height, Desc.MipLevels, Usage, D3DFormat, Pool, &pD3DTex, NULL))) return NULL;
		
		if (!Tex->Create(pD3DTex))
		{
			pD3DTex->Release();
			return NULL;
		}

		if (pData)
		{
			// D3DPOOL_DEFAULT non-D3DUSAGE_DYNAMIC textures can't be locked, but must be
			// modified by calling IDirect3DDevice9::UpdateTexture (from temporary D3DPOOL_SYSTEMMEM texture)
			D3DLOCKED_RECT LockedRect = { 0 };
			if (SUCCEEDED(pD3DTex->LockRect(0, &LockedRect, NULL, D3DLOCK_NOSYSLOCK)))
			{
				DWORD BlockSize = CD3D9DriverFactory::D3DFormatBlockSize(D3DFormat);
				DWORD BlocksH = (BlockSize == 1) ? Height : ((Height + BlockSize - 1) / BlockSize);
				memcpy(LockedRect.pBits, pData, LockedRect.Pitch * BlocksH);
				n_assert(SUCCEEDED(pD3DTex->UnlockRect(0)));
			}
			else
			{
				pD3DTex->Release();
				return NULL;
			}
		}
	}
	else if (Desc.Type == Texture_3D)
	{
		IDirect3DVolumeTexture9* pD3DTex = NULL;
		if (FAILED(pD3DDevice->CreateVolumeTexture(Desc.Width, Desc.Height, Desc.Depth, Desc.MipLevels, Usage, D3DFormat, Pool, &pD3DTex, NULL))) return NULL;
		if (!Tex->Create(pD3DTex))
		{
			pD3DTex->Release();
			return NULL;
		}

		if (pData)
		{
			// D3DPOOL_DEFAULT non-D3DUSAGE_DYNAMIC textures can't be locked, but must be
			// modified by calling IDirect3DDevice9::UpdateTexture (from temporary D3DPOOL_SYSTEMMEM texture)
			D3DLOCKED_BOX LockedBox = { 0 };
			if (SUCCEEDED(pD3DTex->LockBox(0, &LockedBox, NULL, D3DLOCK_NOSYSLOCK)))
			{
				memcpy(LockedBox.pBits, pData, LockedBox.SlicePitch * Desc.Depth);
				n_assert(SUCCEEDED(pD3DTex->UnlockBox(0)));
			}
			else
			{
				pD3DTex->Release();
				return NULL;
			}
		}
	}
	else if (Desc.Type == Texture_Cube)
	{
		IDirect3DCubeTexture9* pD3DTex = NULL;
		if (FAILED(pD3DDevice->CreateCubeTexture(Desc.Width, Desc.MipLevels, Usage, D3DFormat, Pool, &pD3DTex, NULL))) return NULL;
		if (!Tex->Create(pD3DTex))
		{
			pD3DTex->Release();
			return NULL;
		}

		if (pData)
		{
			Sys::Error("CD3D9GPUDriver::CreateTexture() > Cubemap loading face by face - IMPLEMENT ME!\n");
			//!!!faces must be loaded in ECubemapFace/D3D enum order!
		}
	}
	else
	{
		Sys::Error("CD3D9GPUDriver::CreateTexture() > Unknown texture type %d\n", Desc.Type);
		return NULL;
	}

	return Tex.GetUnsafe();
}
//---------------------------------------------------------------------

//???need mips (add to desc)?
//???allow 3D and cubes? will need RT.Create or CreateRenderTarget(Texture, SurfaceLocation)
PRenderTarget CD3D9GPUDriver::CreateRenderTarget(const CRenderTargetDesc& Desc)
{
	D3DFORMAT RTFmt = CD3D9DriverFactory::PixelFormatToD3DFormat(Desc.Format);
	D3DMULTISAMPLE_TYPE MSAAType;
	DWORD MSAAQuality;
	if (!GetD3DMSAAParams(Desc.MSAAQuality, RTFmt, MSAAType, MSAAQuality)) FAIL;

	IDirect3DSurface9* pSurface = NULL;
	IDirect3DTexture9* pTexture = NULL;

	// Using RT as a shader input requires a texture. Since MSAA textures are not supported in D3D9,
	// MSAA RT will be created as a separate surface anyway and then it will be resolved into a texture.

	DWORD Usage = 0;
	if (Desc.UseAsShaderInput)
	{
		UINT Mips = 1;
		if (MSAAType == D3DMULTISAMPLE_NONE)
		{
			Usage |= D3DUSAGE_RENDERTARGET;
			if (Desc.MipLevels != 1)
			{
				Usage |= D3DUSAGE_AUTOGENMIPMAP;
				Mips = Desc.MipLevels; //???or always 0 for D3DUSAGE_AUTOGENMIPMAP?
			}
		}
		if (FAILED(pD3DDevice->CreateTexture(Desc.Width, Desc.Height, Mips, Usage, RTFmt, D3DPOOL_DEFAULT, &pTexture, NULL))) return NULL;
	}

	// Initialize RT surface, use texture level 0 when possible, else create separate surface

	HRESULT hr;
	if (Usage & D3DUSAGE_RENDERTARGET) hr = pTexture->GetSurfaceLevel(0, &pSurface);
	else hr = pD3DDevice->CreateRenderTarget(Desc.Width, Desc.Height, RTFmt, MSAAType, MSAAQuality, FALSE, &pSurface, NULL);

	if (FAILED(hr))
	{
		if (pTexture) pTexture->Release();
		return NULL;
	}

	PD3D9Texture Tex = NULL;
	if (pTexture)
	{
		Tex = n_new(CD3D9Texture);
		if (!Tex->Create(pTexture))
		{
			pSurface->Release();
			pTexture->Release();
			return NULL;
		}
	}

	PD3D9RenderTarget RT = n_new(CD3D9RenderTarget);
	if (!RT->Create(pSurface, Tex))
	{
		Tex = NULL;
		pSurface->Release();
		return NULL;
	}
	return RT.GetUnsafe();
}
//---------------------------------------------------------------------

PDepthStencilBuffer CD3D9GPUDriver::CreateDepthStencilBuffer(const CRenderTargetDesc& Desc)
{
	D3DFORMAT DSFmt = CD3D9DriverFactory::PixelFormatToD3DFormat(Desc.Format);
	D3DMULTISAMPLE_TYPE MSAAType;
	DWORD MSAAQuality;
	if (!GetD3DMSAAParams(Desc.MSAAQuality, DSFmt, MSAAType, MSAAQuality)) FAIL;

	D3DFORMAT BBFmt = CD3D9DriverFactory::PixelFormatToD3DFormat(PixelFmt_DefaultBackBuffer);

	// Make sure the device supports a depth buffer specified
	HRESULT hr = D3D9DrvFactory->GetDirect3D9()->CheckDeviceFormat(	AdapterID,
																	GetD3DDriverType(Type),
																	BBFmt,
																	D3DUSAGE_DEPTHSTENCIL,
																	D3DRTYPE_SURFACE,
																	DSFmt);
	if (FAILED(hr)) return NULL;

	IDirect3DSurface9* pSurface = NULL;
	if (FAILED(pD3DDevice->CreateDepthStencilSurface(Desc.Width, Desc.Height, DSFmt, MSAAType, MSAAQuality, TRUE, &pSurface, NULL))) return NULL;

	if (Desc.UseAsShaderInput)
	{
		// INTZ for reading DS as texture
		// May be unsupported, if so, use another tech like rendering depth into an RT
		//or create depthstencil texture (D fmt, usage DS)
		//need to CheckDeviceFormat (through CheckCaps())
		Sys::Log("Current D3D9 implementation doesn't support UseAsShaderInput for depth-stencil buffers\n");
		pSurface->Release();
		return NULL;
	}

	PD3D9DepthStencilBuffer DS = n_new(CD3D9DepthStencilBuffer);
	if (!DS->Create(pSurface))
	{
		pSurface->Release();
		return NULL;
	}
	return DS.GetUnsafe();
}
//---------------------------------------------------------------------

//???or special Desc?
PRenderState CD3D9GPUDriver::CreateRenderState(const Data::CParams& Desc)
{
	// States supported by D3D9:
	// D3D9-only:
	//D3DRS_SRGBWRITEENABLE             = 194,
	//D3DRS_ALPHATESTENABLE             = 15,
	//D3DRS_ALPHAREF                    = 24,
	//D3DRS_ALPHAFUNC                   = 25,
	//D3DRS_LASTPIXEL                   = 16,
	//D3DRS_DITHERENABLE                = 26,
	//D3DRS_WRAP0-15                    = 128,
	//D3DRS_CLIPPLANEENABLE             = 152,
	//D3DRS_POINTSIZE                   = 154,
	//D3DRS_POINTSIZE_MIN               = 155,
	//D3DRS_POINTSPRITEENABLE           = 156,
	//D3DRS_POINTSCALEENABLE            = 157,
	//D3DRS_POINTSCALE_A                = 158,
	//D3DRS_POINTSCALE_B                = 159,
	//D3DRS_POINTSCALE_C                = 160,
	//D3DRS_POINTSIZE_MAX               = 166,
	//D3DRS_PATCHEDGESTYLE              = 163,
	//D3DRS_DEBUGMONITORTOKEN           = 165,
	//D3DRS_POSITIONDEGREE              = 172,
	//D3DRS_NORMALDEGREE                = 173,
	//D3DRS_MINTESSELLATIONLEVEL        = 178,
	//D3DRS_MAXTESSELLATIONLEVEL        = 179,
	//D3DRS_ADAPTIVETESS_X              = 180,
	//D3DRS_ADAPTIVETESS_Y              = 181,
	//D3DRS_ADAPTIVETESS_Z              = 182,
	//D3DRS_ADAPTIVETESS_W              = 183,
	//D3DRS_ENABLEADAPTIVETESSELLATION  = 184,
	//
	// Rasterizer:
	//D3DRS_FILLMODE                    = 8,
	//D3DRS_CULLMODE                    = 22,
	//D3DRS_MULTISAMPLEANTIALIAS        = 161,
	//D3DRS_MULTISAMPLEMASK             = 162,
	//D3DRS_ANTIALIASEDLINEENABLE       = 176,
	//D3DRS_CLIPPING                    = 136, //???is as DX11 DepthClipEnable?
	//D3DRS_DEPTHBIAS                   = 195,
	//D3DRS_SLOPESCALEDEPTHBIAS         = 175,
	//D3DRS_SCISSORTESTENABLE           = 174,
	//
	// Depth-stencil:
	//D3DRS_ZENABLE                     = 7,	// bool + D3DZB_USEW
	//D3DRS_ZWRITEENABLE                = 14,
	//D3DRS_ZFUNC                       = 23,
	//D3DRS_STENCILENABLE               = 52,
	//D3DRS_STENCILFAIL                 = 53,
	//D3DRS_STENCILZFAIL                = 54,
	//D3DRS_STENCILPASS                 = 55,
	//D3DRS_STENCILFUNC                 = 56,
	//D3DRS_STENCILREF                  = 57,
	//D3DRS_STENCILMASK                 = 58,
	//D3DRS_STENCILWRITEMASK            = 59,
	//D3DRS_TWOSIDEDSTENCILMODE         = 185,
	//D3DRS_CCW_STENCILFAIL             = 186,
	//D3DRS_CCW_STENCILZFAIL            = 187,
	//D3DRS_CCW_STENCILPASS             = 188,
	//D3DRS_CCW_STENCILFUNC             = 189,
	//
	// Blend:
	//D3DRS_SRCBLEND                    = 19,
	//D3DRS_DESTBLEND                   = 20,
	//D3DRS_ALPHABLENDENABLE            = 27,
	//D3DRS_TEXTUREFACTOR               = 60, // Color for blending
	//D3DRS_BLENDOP                     = 171,
	//D3DRS_SEPARATEALPHABLENDENABLE    = 206,
	//D3DRS_SRCBLENDALPHA               = 207,
	//D3DRS_DESTBLENDALPHA              = 208,
	//D3DRS_BLENDOPALPHA                = 209
	//D3DRS_COLORWRITEENABLE            = 168,
	//D3DRS_COLORWRITEENABLE1           = 190,
	//D3DRS_COLORWRITEENABLE2           = 191,
	//D3DRS_COLORWRITEENABLE3           = 192,
	//D3DRS_BLENDFACTOR                 = 193,

	IDirect3DVertexShader9* pVS = NULL;
	IDirect3DPixelShader9* pPS = NULL;

	// try to find each shader already loaded (some compiled shader file)
	// if not loaded, load and add into the cache
	//???how to determine final compiled shader file? when compile my effect, serialize it
	//with final shader file names? can even use DSS for effects!

	PD3D9RenderState RS = n_new(CD3D9RenderState);
	RS->pVS = pVS;
	RS->pPS = pPS;

	RenderStates.Add(RS);

	return RS.GetUnsafe();
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode)
{
	n_assert_dbg(Resource.IsA<CD3D9VertexBuffer>());
	if (!ppOutData) FAIL;

	IDirect3DVertexBuffer9* pVB = ((const CD3D9VertexBuffer&)Resource).GetD3DBuffer();
	if (!pVB) FAIL;

	//???increment internal lock count of resource?
	//???assert or check CPU access?! or Resource.CanMap()?

	return SUCCEEDED(pVB->Lock(0, 0, ppOutData, GetD3DLockFlags(Mode)));
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode)
{
	n_assert_dbg(Resource.IsA<CD3D9IndexBuffer>());
	if (!ppOutData) FAIL;

	IDirect3DIndexBuffer9* pIB = ((const CD3D9IndexBuffer&)Resource).GetD3DBuffer();
	if (!pIB) FAIL;

	//???increment internal lock count of resource?
	//???assert or check CPU access?! or Resource.CanMap()?

	return SUCCEEDED(pIB->Lock(0, 0, ppOutData, GetD3DLockFlags(Mode)));
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, DWORD ArraySlice, DWORD MipLevel)
{
	n_assert_dbg(Resource.IsA<CD3D9Texture>());
	IDirect3DBaseTexture9* pD3DBaseTex = ((const CD3D9Texture&)Resource).GetD3DBaseTexture();
	if (!pD3DBaseTex) FAIL;

	//???increment internal lock count of resource?
	//???assert or check CPU access?! or Resource.CanMap()?

	switch (Resource.GetDesc().Type)
	{
		case Texture_1D:
		case Texture_2D:
		{
			IDirect3DTexture9* pD3DTex = (IDirect3DTexture9*)pD3DBaseTex;
			D3DLOCKED_RECT D3DRect;
			if (FAILED(pD3DTex->LockRect(MipLevel, &D3DRect, NULL, GetD3DLockFlags(Mode)))) FAIL;

			OutData.pData = (char*)D3DRect.pBits;
			OutData.RowPitch = (DWORD)D3DRect.Pitch;
			OutData.SlicePitch = 0;

			OK;
		}

		case Texture_3D:
		{
			IDirect3DVolumeTexture9* pD3DTex = (IDirect3DVolumeTexture9*)pD3DBaseTex;
			D3DLOCKED_BOX D3DBox;
			if (FAILED(pD3DTex->LockBox(MipLevel, &D3DBox, NULL, GetD3DLockFlags(Mode)))) FAIL;

			OutData.pData = (char*)D3DBox.pBits;
			OutData.RowPitch = (DWORD)D3DBox.RowPitch;
			OutData.SlicePitch = (DWORD)D3DBox.SlicePitch;

			OK;
		}

		case Texture_Cube:
		{
			IDirect3DCubeTexture9* pD3DTex = (IDirect3DCubeTexture9*)pD3DBaseTex;
			D3DLOCKED_RECT D3DRect;
			if (FAILED(pD3DTex->LockRect(GetD3DCubeMapFace((ECubeMapFace)ArraySlice), MipLevel, &D3DRect, NULL, GetD3DLockFlags(Mode)))) FAIL;

			OutData.pData = (char*)D3DRect.pBits;
			OutData.RowPitch = (DWORD)D3DRect.Pitch;
			OutData.SlicePitch = 0;

			OK;
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::UnmapResource(const CVertexBuffer& Resource)
{
	n_assert_dbg(Resource.IsA<CD3D9VertexBuffer>());
	//???!!!return are outstanding locks or resource was unlocked?!
	IDirect3DVertexBuffer9* pVB = ((const CD3D9VertexBuffer&)Resource).GetD3DBuffer();
	if (!pVB) FAIL;
	return SUCCEEDED(pVB->Unlock());
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::UnmapResource(const CIndexBuffer& Resource)
{
	n_assert_dbg(Resource.IsA<CD3D9IndexBuffer>());
	//???!!!return are outstanding locks or resource was unlocked?!
	IDirect3DIndexBuffer9* pIB = ((const CD3D9IndexBuffer&)Resource).GetD3DBuffer();
	if (!pIB) FAIL;
	return SUCCEEDED(pIB->Unlock());
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::UnmapResource(const CTexture& Resource, DWORD ArraySlice, DWORD MipLevel)
{
	n_assert_dbg(Resource.IsA<CD3D9Texture>());
	IDirect3DBaseTexture9* pD3DBaseTex = ((const CD3D9Texture&)Resource).GetD3DBaseTexture();
	if (!pD3DBaseTex) FAIL;

	//???!!!return are outstanding locks or resource was unlocked?!
	switch (Resource.GetDesc().Type)
	{
		case Texture_1D:
		case Texture_2D:
			return SUCCEEDED(((IDirect3DTexture9*)pD3DBaseTex)->UnlockRect(MipLevel));

		case Texture_3D:
			return SUCCEEDED(((IDirect3DVolumeTexture9*)pD3DBaseTex)->UnlockBox(MipLevel));

		case Texture_Cube:
			return SUCCEEDED(((IDirect3DCubeTexture9*)pD3DBaseTex)->UnlockRect(GetD3DCubeMapFace((ECubeMapFace)ArraySlice), MipLevel));
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::ReadFromResource(void* pDest, const CVertexBuffer& Resource, DWORD Size, DWORD Offset)
{
	n_assert_dbg(Resource.IsA<CD3D9VertexBuffer>());
	const CD3D9VertexBuffer& VB9 = (const CD3D9VertexBuffer&)Resource;
	IDirect3DVertexBuffer9* pBuf = VB9.GetD3DBuffer();
	if (!pBuf || !pDest) FAIL;

	DWORD BufferSize = VB9.GetSizeInBytes();
	DWORD RequestedSize = Size ? Size : BufferSize;
	DWORD SizeToCopy = n_min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	char* pSrc = NULL;
	if (FAILED(pBuf->Lock(Offset, SizeToCopy, (void**)&pSrc, D3DLOCK_READONLY))) FAIL;
	memcpy(pDest, pSrc, SizeToCopy);
	if (FAILED(pBuf->Unlock())) FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::ReadFromResource(void* pDest, const CIndexBuffer& Resource, DWORD Size, DWORD Offset)
{
	n_assert_dbg(Resource.IsA<CD3D9IndexBuffer>());
	const CD3D9IndexBuffer& IB9 = (const CD3D9IndexBuffer&)Resource;
	IDirect3DIndexBuffer9* pBuf = IB9.GetD3DBuffer();
	if (!pBuf || !pDest) FAIL;

	DWORD BufferSize = IB9.GetSizeInBytes();
	DWORD RequestedSize = Size ? Size : BufferSize;
	DWORD SizeToCopy = n_min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	char* pSrc = NULL;
	if (FAILED(pBuf->Lock(Offset, SizeToCopy, (void**)&pSrc, D3DLOCK_READONLY))) FAIL;
	memcpy(pDest, pSrc, SizeToCopy);
	if (FAILED(pBuf->Unlock())) FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::ReadFromResource(const CImageData& Dest, const CTexture& Resource, DWORD ArraySlice, DWORD MipLevel, const Data::CBox* pRegion)
{
	n_assert_dbg(Resource.IsA<CD3D9Texture>());

	const CTextureDesc& Desc = Resource.GetDesc();
	DWORD Dims = Resource.GetDimensionCount();
	if (!Dest.pData || !Dims || MipLevel >= Desc.MipLevels || (Desc.Type == Texture_Cube && ArraySlice > 5)) FAIL;

	UINT Usage = ((const CD3D9Texture&)Resource).GetD3DUsage();
	const bool IsNonMappable = (((const CD3D9Texture&)Resource).GetD3DPool() == D3DPOOL_DEFAULT);
	if (IsNonMappable && !(Usage & D3DUSAGE_RENDERTARGET)) FAIL;

	D3DFORMAT D3DFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Desc.Format);

	// No format conversion for now, src & dest texel formats must match
	DWORD BPP = CD3D9DriverFactory::D3DFormatBitsPerPixel(D3DFormat);
	if (!BPP) FAIL;

	DWORD TotalSizeX = n_max(Desc.Width >> MipLevel, 1);
	DWORD TotalSizeY = n_max(Desc.Height >> MipLevel, 1);
	DWORD TotalSizeZ = n_max(Desc.Depth >> MipLevel, 1);

	CCopyImageParams Params;
	Params.BitsPerPixel = BPP;

	if (!CalcValidImageRegion(pRegion, Dims, TotalSizeX, TotalSizeY, TotalSizeZ,
							  Params.Offset[0], Params.Offset[1], Params.Offset[2],
							  Params.CopySize[0], Params.CopySize[1], Params.CopySize[2]))
	{
		OK;
	}

	CImageData SrcData;
	IDirect3DSurface9* pOffscreenSurf = NULL;
	if (IsNonMappable)
	{
		IDirect3DSurface9* pSrcSurf = NULL;
		if (Desc.Type == Texture_1D || Desc.Type == Texture_2D)
		{
			IDirect3DTexture9* pTex = ((const CD3D9Texture&)Resource).GetD3DTexture();
			if (FAILED(pTex->GetSurfaceLevel(MipLevel, &pSrcSurf)))  FAIL;
		}
		else if (Desc.Type == Texture_Cube)
		{
			IDirect3DCubeTexture9* pTex = ((const CD3D9Texture&)Resource).GetD3DCubeTexture();
			if (FAILED(pTex->GetCubeMapSurface(GetD3DCubeMapFace((ECubeMapFace)ArraySlice), MipLevel, &pSrcSurf))) FAIL;
		}
		else Sys::Error("CD3D9GPUDriver::ReadFromResource() > Unsupported non-mappable texture type\n");

		if (FAILED(pD3DDevice->CreateOffscreenPlainSurface(TotalSizeX, TotalSizeY, D3DFormat, D3DPOOL_SYSTEMMEM, &pOffscreenSurf, NULL)))
		{
			pSrcSurf->Release();
			FAIL;
		}

		const bool Result = SUCCEEDED(pD3DDevice->GetRenderTargetData(pSrcSurf, pOffscreenSurf));
			
		pSrcSurf->Release();
			
		if (!Result)
		{
			pOffscreenSurf->Release();
			FAIL;
		}

		D3DLOCKED_RECT D3DRect;
		if (FAILED(pOffscreenSurf->LockRect(&D3DRect, NULL, 0)))
		{
			pOffscreenSurf->Release();
			FAIL;
		}
		SrcData.pData = (char*)D3DRect.pBits;
		SrcData.RowPitch = D3DRect.Pitch;
	}
	else
	{
		if (!MapResource(SrcData, Resource, Map_Read, ArraySlice, MipLevel)) FAIL;
	}

	Params.TotalSize[0] = TotalSizeX;
	Params.TotalSize[1] = TotalSizeY;

	DWORD ImageCopyFlags = CopyImage_AdjustSrc;
	if (CD3D9DriverFactory::D3DFormatBlockSize(D3DFormat) > 1)
		ImageCopyFlags |= CopyImage_BlockCompressed;
	if (Desc.Type == Texture_3D) ImageCopyFlags |= CopyImage_3DImage;

	CopyImage(SrcData, Dest, ImageCopyFlags, Params);

	bool Result;
	if (IsNonMappable)
	{
		Result = SUCCEEDED(pOffscreenSurf->UnlockRect());
		pOffscreenSurf->Release();
	}
	else
	{
		Result = UnmapResource(Resource, ArraySlice, MipLevel);
	}

	return Result;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::WriteToResource(CVertexBuffer& Resource, const void* pData, DWORD Size, DWORD Offset)
{
	n_assert_dbg(Resource.IsA<CD3D9VertexBuffer>());
	const CD3D9VertexBuffer& VB9 = (const CD3D9VertexBuffer&)Resource;
	IDirect3DVertexBuffer9* pBuf = VB9.GetD3DBuffer();
	if (!pBuf || !pData) FAIL;

	DWORD BufferSize = VB9.GetSizeInBytes();
	DWORD RequestedSize = Size ? Size : BufferSize;
	DWORD SizeToCopy = n_min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	const int UpdateWhole = (!Offset && SizeToCopy == BufferSize);

	//???check both pointers to be align-16?
	UINT LockFlags = ((VB9.GetD3DUsage() & D3DUSAGE_DYNAMIC) && UpdateWhole) ? D3DLOCK_DISCARD : 0;
	char* pDest = NULL;
	if (FAILED(pBuf->Lock(Offset, SizeToCopy, (void**)&pDest, LockFlags))) FAIL;
	memcpy(pDest, pData, SizeToCopy);
	if (FAILED(pBuf->Unlock())) FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::WriteToResource(CIndexBuffer& Resource, const void* pData, DWORD Size, DWORD Offset)
{
	n_assert_dbg(Resource.IsA<CD3D9IndexBuffer>());
	const CD3D9IndexBuffer& IB9 = (const CD3D9IndexBuffer&)Resource;

	IDirect3DIndexBuffer9* pBuf = IB9.GetD3DBuffer();
	if (!pBuf || !pData) FAIL;

	DWORD BufferSize = IB9.GetSizeInBytes();
	DWORD RequestedSize = Size ? Size : BufferSize;
	DWORD SizeToCopy = n_min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	const int UpdateWhole = (!Offset && SizeToCopy == BufferSize);

	//???check both pointers to be align-16?
	UINT LockFlags = ((IB9.GetD3DUsage() & D3DUSAGE_DYNAMIC) && UpdateWhole) ? D3DLOCK_DISCARD : 0;
	char* pDest = NULL;
	if (FAILED(pBuf->Lock(Offset, SizeToCopy, (void**)&pDest, LockFlags))) FAIL;
	memcpy(pDest, pData, SizeToCopy);
	if (FAILED(pBuf->Unlock())) FAIL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::WriteToResource(CTexture& Resource, const CImageData& SrcData, DWORD ArraySlice, DWORD MipLevel, const Data::CBox* pRegion)
{
	n_assert_dbg(Resource.IsA<CD3D9Texture>());

	const CTextureDesc& Desc = Resource.GetDesc();
	DWORD Dims = Resource.GetDimensionCount();
	if (!SrcData.pData || !Dims || MipLevel >= Desc.MipLevels || (Desc.Type == Texture_Cube && ArraySlice > 5)) FAIL;

	D3DFORMAT D3DFormat = CD3D9DriverFactory::PixelFormatToD3DFormat(Desc.Format);

	// No format conversion for now, src & dest texel formats must match
	DWORD BPP = CD3D9DriverFactory::D3DFormatBitsPerPixel(D3DFormat);
	if (!BPP) FAIL;

	DWORD TotalSizeX = n_max(Desc.Width >> MipLevel, 1);
	DWORD TotalSizeY = n_max(Desc.Height >> MipLevel, 1);
	DWORD TotalSizeZ = n_max(Desc.Depth >> MipLevel, 1);

	CCopyImageParams Params;
	Params.BitsPerPixel = BPP;

	DWORD OffsetX, OffsetY, SizeX, SizeY;
	if (!CalcValidImageRegion(pRegion, Dims, TotalSizeX, TotalSizeY, TotalSizeZ,
							  OffsetX, OffsetY, Params.Offset[2], SizeX, SizeY, Params.CopySize[2]))
	{
		OK;
	}

	CImageData DestData;

	IDirect3DTexture9* pSrcTex = NULL;
	IDirect3DSurface9* pSrcSurf = NULL;
	IDirect3DSurface9* pDestSurf = NULL;
	UINT Usage = ((const CD3D9Texture&)Resource).GetD3DUsage();
	const bool IsNonMappable = (((const CD3D9Texture&)Resource).GetD3DPool() == D3DPOOL_DEFAULT);
	if (IsNonMappable)
	{
		if (Desc.Type == Texture_1D || Desc.Type == Texture_2D)
		{
			IDirect3DTexture9* pTex = ((const CD3D9Texture&)Resource).GetD3DTexture();
			if (FAILED(pTex->GetSurfaceLevel(MipLevel, &pDestSurf)))  FAIL;
		}
		else if (Desc.Type == Texture_Cube)
		{
			IDirect3DCubeTexture9* pTex = ((const CD3D9Texture&)Resource).GetD3DCubeTexture();
			if (FAILED(pTex->GetCubeMapSurface(GetD3DCubeMapFace((ECubeMapFace)ArraySlice), MipLevel, &pDestSurf))) FAIL;
		}
		else Sys::Error("CD3D9GPUDriver::WriteToResource() > Unsupported non-mappable texture type\n");

		if (Usage & D3DUSAGE_RENDERTARGET)
		{
			if (FAILED(pD3DDevice->CreateOffscreenPlainSurface(TotalSizeX, TotalSizeY, D3DFormat, D3DPOOL_SYSTEMMEM, &pSrcSurf, NULL)))
			{
				pDestSurf->Release();
				FAIL;
			}
		}
		else
		{
			if (FAILED(pD3DDevice->CreateTexture(TotalSizeX, TotalSizeY, 1, 0, D3DFormat, D3DPOOL_SYSTEMMEM, &pSrcTex, NULL)))
			{
				pDestSurf->Release();
				FAIL;
			}
			if (FAILED(pSrcTex->GetSurfaceLevel(0, &pSrcSurf)))
			{
				pSrcTex->Release();
				pDestSurf->Release();
				FAIL;
			}
		}

		D3DLOCKED_RECT D3DRect;
		if (FAILED(pSrcSurf->LockRect(&D3DRect, NULL, 0)))
		{
			pSrcSurf->Release();
			if (pSrcTex) pSrcTex->Release();
			pDestSurf->Release();
			FAIL;
		}
		DestData.pData = (char*)D3DRect.pBits;
		DestData.RowPitch = D3DRect.Pitch;
	}
	else
	{
		EResourceMapMode Mode;
		if (Usage & D3DUSAGE_DYNAMIC)
		{
			const bool UpdateWhole =
				!pRegion || (SizeX == TotalSizeX && (Dims < 2 || SizeY == TotalSizeY && (Dims < 3 || Params.CopySize[2] == TotalSizeZ)));
			Mode = UpdateWhole ? Map_WriteDiscard : Map_Write;
		}
		else Mode = Map_Write;

		if (!MapResource(DestData, Resource, Mode, ArraySlice, MipLevel)) FAIL;
	}

	Params.Offset[0] = OffsetX;
	Params.Offset[1] = OffsetY;
	Params.CopySize[0] = SizeX;
	Params.CopySize[1] = SizeY;
	Params.TotalSize[0] = TotalSizeX;
	Params.TotalSize[1] = TotalSizeY;

	DWORD ImageCopyFlags = CopyImage_AdjustDest;
	if (CD3D9DriverFactory::D3DFormatBlockSize(D3DFormat) > 1)
		ImageCopyFlags |= CopyImage_BlockCompressed;
	if (Desc.Type == Texture_3D) ImageCopyFlags |= CopyImage_3DImage;

	CopyImage(SrcData, DestData, ImageCopyFlags, Params);

	bool Result;
	if (IsNonMappable)
	{
		if (SUCCEEDED(pSrcSurf->UnlockRect()))
		{
			RECT r = { OffsetX, OffsetY, OffsetX + SizeX, OffsetY + SizeY };
			POINT p = { OffsetX, OffsetY };
			Result = SUCCEEDED(pD3DDevice->UpdateSurface(pSrcSurf, &r, pDestSurf, &p));
		}
		else Result = false;

		pDestSurf->Release();
		pSrcSurf->Release();
		if (pSrcTex) pSrcTex->Release();
	}
	else
	{
		Result = UnmapResource(Resource, ArraySlice, MipLevel);
	}

	return Result;
}
//---------------------------------------------------------------------

D3DDEVTYPE CD3D9GPUDriver::GetD3DDriverType(EGPUDriverType DriverType)
{
	switch (DriverType)
	{
		case GPU_AutoSelect:	Sys::Error("CD3D9GPUDriver::GetD3DDriverType() > GPU_AutoSelect isn't an actual GPU driver type"); return D3DDEVTYPE_HAL;
		case GPU_Hardware:		return D3DDEVTYPE_HAL;
		case GPU_Reference:		return D3DDEVTYPE_REF;
		case GPU_Software:		return D3DDEVTYPE_SW;
		case GPU_Null:			return D3DDEVTYPE_NULLREF;
		default:				Sys::Error("CD3D9GPUDriver::GetD3DDriverType() > invalid GPU driver type"); return D3DDEVTYPE_HAL;
	};
}
//---------------------------------------------------------------------

void CD3D9GPUDriver::GetUsagePool(DWORD InAccessFlags, DWORD& OutUsage, D3DPOOL& OutPool)
{
	Data::CFlags AccessFlags(InAccessFlags);
	if (AccessFlags.IsNot(Access_CPU_Write | Access_CPU_Read))
	{
		OutPool = D3DPOOL_MANAGED; //!!!set default if resmgr will manage reloading!
		OutUsage = 0;
	}
	else if (InAccessFlags == (Access_GPU_Read | Access_CPU_Write))
	{
		OutPool = D3DPOOL_DEFAULT;
		OutUsage = D3DUSAGE_DYNAMIC;
	}
	else
	{
		OutPool = D3DPOOL_SYSTEMMEM;
		OutUsage = D3DUSAGE_DYNAMIC; //???has meaning?
	}
}
//---------------------------------------------------------------------

UINT CD3D9GPUDriver::GetD3DLockFlags(EResourceMapMode MapMode)
{
	switch (MapMode)
	{
		case Map_Read:				return D3DLOCK_READONLY;
		case Map_Write:				return 0;
		case Map_ReadWrite:			return 0;
		case Map_WriteDiscard:		return D3DLOCK_DISCARD;
		case Map_WriteNoOverwrite:	return D3DLOCK_NOOVERWRITE;
		default: Sys::Error("CD3D9GPUDriver::GetD3DLockFlags() > Invalid map mode\n"); return 0;
	}
}
//---------------------------------------------------------------------

D3DCUBEMAP_FACES CD3D9GPUDriver::GetD3DCubeMapFace(ECubeMapFace Face)
{
	switch (Face)
	{
		case CubeFace_PosX:	return D3DCUBEMAP_FACE_POSITIVE_X;
		case CubeFace_NegX:	return D3DCUBEMAP_FACE_NEGATIVE_X;
		case CubeFace_PosY:	return D3DCUBEMAP_FACE_POSITIVE_Y;
		case CubeFace_NegY:	return D3DCUBEMAP_FACE_NEGATIVE_Y;
		case CubeFace_PosZ:	return D3DCUBEMAP_FACE_POSITIVE_Z;
		case CubeFace_NegZ:	return D3DCUBEMAP_FACE_NEGATIVE_Z;
		default: Sys::Error("CD3D9GPUDriver::GetD3DCubeMapFace() > Invalid cubemap face\n"); return D3DCUBEMAP_FACE_FORCE_DWORD;
	}
}
//---------------------------------------------------------------------

//???bool Windowed as parameter and re-check available MSAA on fullscreen transitions?
bool CD3D9GPUDriver::GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT Format, D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality) const
{
#if DEM_RENDER_DEBUG
	OutType = D3DMULTISAMPLE_NONE;
	OutQuality = 0;
	OK;
#else
	switch (MSAA)
	{
		case MSAA_None:	OutType = D3DMULTISAMPLE_NONE; break;
		case MSAA_2x:	OutType = D3DMULTISAMPLE_2_SAMPLES; break;
		case MSAA_4x:	OutType = D3DMULTISAMPLE_4_SAMPLES; break;
		case MSAA_8x:	OutType = D3DMULTISAMPLE_8_SAMPLES; break;
	};

	DWORD QualLevels = 0;
	HRESULT hr = D3D9DrvFactory->GetDirect3D9()->CheckDeviceMultiSampleType(AdapterID,
																			GetD3DDriverType(Type),
																			Format,
																			FALSE, // Windowed
																			OutType,
																			&QualLevels);
	if (hr == D3DERR_NOTAVAILABLE)
	{
		OutType = D3DMULTISAMPLE_NONE;
		OutQuality = 0;
		FAIL;
	}
	n_assert(SUCCEEDED(hr));

	OutQuality = QualLevels ? QualLevels - 1 : 0;

	OK;
#endif
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	for (int i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D9SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			DestroySwapChain(i);
			OK; // Only one swap chain is allowed for each window
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	for (int i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D9SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			// May not fire in fullscreen mode by design, this assert checks it
			// If assertion failed, we should rewrite this handler and maybe some other code
			n_assert_dbg(!SC.IsFullscreen());

			ResizeSwapChain(i, pWnd->GetWidth(), pWnd->GetHeight());
			OK; // Only one swap chain is allowed for each window
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	for (int i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D9SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			if (SC.IsFullscreen()) n_assert(SwitchToWindowed(i));
			else n_assert(SwitchToFullscreen(i));
			OK;
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D9GPUDriver::OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
#ifdef _DEBUG // Check that it is fullscreen and SC is implicit
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	Sys::DbgOut("CD3D9GPUDriver::OnOSWindowPaint() from %s\n", pWnd->GetTitle());
	for (int i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D9SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			n_assert(SC.IsFullscreen() && !SC.pSwapChain);
			break;
		}
	}
#endif

	n_assert_dbg(pD3DDevice);
	pD3DDevice->Present(NULL, NULL, NULL, NULL);
	OK;
}
//---------------------------------------------------------------------

}
