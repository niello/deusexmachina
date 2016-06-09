#include "D3D11GPUDriver.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11DisplayDriver.h>
#include <Render/D3D11/D3D11VertexLayout.h>
#include <Render/D3D11/D3D11VertexBuffer.h>
#include <Render/D3D11/D3D11IndexBuffer.h>
#include <Render/D3D11/D3D11Texture.h>
#include <Render/D3D11/D3D11RenderTarget.h>
#include <Render/D3D11/D3D11DepthStencilBuffer.h>
#include <Render/D3D11/D3D11RenderState.h>
#include <Render/D3D11/D3D11ConstantBuffer.h>
#include <Render/D3D11/D3D11Sampler.h>
#include <Render/D3D11/D3D11Shader.h>
#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
#include <Render/ImageUtils.h>
#include <Events/EventServer.h>
#include <System/OSWindow.h>
#include <Data/StringID.h>
#include <Core/Factory.h>
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

//!!!D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE!
//each scissor rect belongs to a viewport

namespace Render
{
__ImplementClass(Render::CD3D11GPUDriver, '11GD', Render::CGPUDriver);

bool CD3D11GPUDriver::Init(UPTR AdapterNumber, EGPUDriverType DriverType)
{
	if (!CGPUDriver::Init(AdapterNumber, DriverType)) FAIL;

	n_assert(AdapterID == Adapter_AutoSelect || D3D11DrvFactory->AdapterExists(AdapterID));

	//!!!fix if will be multithreaded, forex job-based!
	UPTR CreateFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

#if !defined(_DEBUG) && (DEM_RENDER_DEBUG == 0)
	// Prevents end-users from debugging an application
	CreateFlags |= D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY;
#elif (DEM_RENDER_DEBUG != 0)
	// Enable D3D11 debug layer only in debug builds with render debugging on
	CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
//const char c_szName[] = "mytexture.jpg";
//pTexture->SetPrivateData( WKPDID_D3DDebugObjectName, sizeof( c_szName ) - 1, c_szName );
	// D3D11.1: D3D11_CREATE_DEVICE_DEBUGGABLE - shader debugging
	//Shader debugging requires a driver that is implemented to the WDDM for Windows 8 (WDDM 1.2)
#endif

	// If we use NULL adapter (Adapter_AutoSelect), new DXGI factory will be created. We avoid it.
	IDXGIAdapter1* pAdapter = NULL;
	if (AdapterID == Adapter_AutoSelect) AdapterID = 0;
	if (!SUCCEEDED(D3D11DrvFactory->GetDXGIFactory()->EnumAdapters1(AdapterID, &pAdapter))) FAIL;

	// NB: All mapped pointers will be SSE-friendly 16-byte aligned for these feature levels, and other
	// code relies on it. Adding feature levels 9.x may require to change resource mapping code.
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		//D3D_FEATURE_LEVEL_11_1, //!!!Can use D3D11.1 and DXGI 1.2 API on Win 7 with platform update!
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	UPTR FeatureLevelCount = sizeof_array(FeatureLevels);

	D3D_FEATURE_LEVEL D3DFeatureLevel;

	HRESULT hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, CreateFlags,
								   FeatureLevels, FeatureLevelCount, D3D11_SDK_VERSION,
								   &pD3DDevice, &D3DFeatureLevel, &pD3DImmContext);

	//if (hr == E_INVALIDARG)
	//{
	//	// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
	//	hr = D3D11CreateDevice(	pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, CreateFlags, FeatureLevels + 1, FeatureLevelCount - 1,
	//							D3D11_SDK_VERSION, &pD3DDevice, &D3DFeatureLevel, &pD3DImmContext);
	//}

	pAdapter->Release();

	if (FAILED(hr))
	{
		Sys::Log("Failed to create Direct3D11 device object, hr = 0x%x!\n", hr);
		FAIL;
	}

	if (AdapterID == 0) Type = GPU_Hardware; //???else? //!!!in D3D9 type was in device caps!

	Sys::Log("Device created: %s, feature level 0x%x\n", "HAL", (int)D3DFeatureLevel);

	UPTR MRTCountCaps = 0;
	UPTR MaxViewportCountCaps = 0;
	switch (D3DFeatureLevel)
	{
		case D3D_FEATURE_LEVEL_9_1:
			FeatureLevel = GPU_Level_D3D9_1;
			MRTCountCaps = D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT;
			MaxViewportCountCaps = 1;
			break;
		case D3D_FEATURE_LEVEL_9_2:
			FeatureLevel = GPU_Level_D3D9_2;
			MRTCountCaps = D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT;
			MaxViewportCountCaps = 1;
			break;
		case D3D_FEATURE_LEVEL_9_3:
			FeatureLevel = GPU_Level_D3D9_3;
			MRTCountCaps = D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT;
			MaxViewportCountCaps = 1;
			break;
		case D3D_FEATURE_LEVEL_10_0:
			FeatureLevel = GPU_Level_D3D10_0;
			MRTCountCaps = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
			MaxViewportCountCaps = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
			break;
		case D3D_FEATURE_LEVEL_10_1:
			FeatureLevel = GPU_Level_D3D10_1;
			MRTCountCaps = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
			MaxViewportCountCaps = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
			break;
		case D3D_FEATURE_LEVEL_11_0:
			FeatureLevel = GPU_Level_D3D11_0;
			MRTCountCaps = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
			MaxViewportCountCaps = D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
			break;
		//case D3D_FEATURE_LEVEL_11_1:
		default:
			FeatureLevel = GPU_Level_D3D11_1;
			MRTCountCaps = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
			MaxViewportCountCaps = D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
			break;
	}

	UPTR MaxVertexStreamsCaps = GetMaxVertexStreams();

	CurrRT.SetSize(MRTCountCaps);
	CurrVB.SetSize(MaxVertexStreamsCaps);
	CurrVBOffset.SetSize(MaxVertexStreamsCaps);
	CurrCB.SetSize(ShaderType_COUNT * D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
	CurrSS.SetSize(ShaderType_COUNT * D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT);

	// To determine whether viewport is set or not at a particular index, we use a bitfield.
	// So we are not only limited by a D3D11 caps but also by a bit count in a mask.
	//???!!!use each bit for pair, is there any reason to set VP & SR separately?!
	//???and also one dirty flag? when set VP do D3D11 remember its SR or it resets SR size?
	//!!!need static assert!
	n_assert_dbg(sizeof(VPSRSetFlags) * 4 >= VP_OR_SR_SET_FLAG_COUNT);
	MaxViewportCount = n_min(MaxViewportCountCaps, VP_OR_SR_SET_FLAG_COUNT);
	CurrVP = n_new_array(D3D11_VIEWPORT, MaxViewportCount);
	CurrSR = n_new_array(RECT, MaxViewportCount);
	VPSRSetFlags.ClearAll();

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::InitSwapChainRenderTarget(CD3D11SwapChain& SC)
{
	n_assert(SC.pSwapChain);

	// Already initialized, can't reinitialize
	if (SC.BackBufferRT.IsValidPtr() && SC.BackBufferRT->IsValid()) FAIL;

	ID3D11Texture2D* pBackBuffer = NULL;
	if (FAILED(SC.pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) FAIL;
	
	ID3D11RenderTargetView* pRTV = NULL;
	HRESULT hr = pD3DDevice->CreateRenderTargetView(pBackBuffer, NULL, &pRTV);
	pBackBuffer->Release();
	if (FAILED(hr)) FAIL;

	if (!SC.BackBufferRT.IsValidPtr()) SC.BackBufferRT = n_new(CD3D11RenderTarget);
	if (!SC.BackBufferRT->As<CD3D11RenderTarget>()->Create(pRTV, NULL))
	{
		pRTV->Release();
		FAIL;
	}

	OK;
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::Release()
{
	if (!pD3DDevice) return;

	VertexLayouts.Clear();
	RenderStates.Clear();
	Samplers.Clear();

	//!!!if code won't be reused in Reset(), call DestroySwapChain()!
	for (UPTR i = 0; i < SwapChains.GetCount() ; ++i)
		if (SwapChains[i].IsValid()) SwapChains[i].Destroy();

	SAFE_DELETE_ARRAY(CurrVP);
	SAFE_DELETE_ARRAY(CurrSR);
	CurrSRV.Clear();
	CurrVB.SetSize(0);
	CurrVBOffset.SetSize(0);
	CurrCB.SetSize(0);
	CurrSS.SetSize(0);
	CurrRT.SetSize(0);

	//ctx->ClearState()
	//!!!UnbindD3DResources();
	//!!!can call the same event as on lost device!

	//for (int i = 1; i < MaxRenderTargetCount; ++i)
	//	pD3DDevice->SetRenderTarget(i, NULL);
	//pD3DDevice->SetDepthStencilSurface(NULL); //???need when auto depth stencil?

	//EventSrv->FireEvent(CStrID("OnRenderDeviceRelease"));

	//!!!ReleaseQueries();

	SAFE_RELEASE(pD3DImmContext);
	SAFE_RELEASE(pD3DDevice);

//	IsInsideFrame = false;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::CheckCaps(ECaps Cap) const
{
	n_assert(pD3DDevice);

//pD3DDevice->GetFeatureLevel()
//CheckFeatureSupport
//CheckFormatSupport

	switch (Cap)
	{
		case Caps_VSTex_L16:
		case Caps_VSTexFiltering_Linear:
		case Caps_ReadDepthAsTexture:		OK;
	}

	FAIL;
}
//---------------------------------------------------------------------

UPTR CD3D11GPUDriver::GetMaxVertexStreams() const
{
	D3D_FEATURE_LEVEL FeatLevel = pD3DDevice->GetFeatureLevel();
	if (FeatLevel >= D3D_FEATURE_LEVEL_11_0) return D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	else if (FeatLevel >= D3D_FEATURE_LEVEL_10_0) return D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	else return 4; // Relatively safe value
}
//---------------------------------------------------------------------

UPTR CD3D11GPUDriver::GetMaxTextureSize(ETextureType Type) const
{
	switch (pD3DDevice->GetFeatureLevel())
	{
		case D3D_FEATURE_LEVEL_9_1:
		case D3D_FEATURE_LEVEL_9_2:
		{
			switch (Type)
			{
				case Texture_1D: return D3D_FL9_1_REQ_TEXTURE1D_U_DIMENSION;
				case Texture_2D: return D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION;
				case Texture_3D: return D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
				case Texture_Cube: return D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION;
				default: return 0;
			}
		}

		case D3D_FEATURE_LEVEL_9_3:
		{
			switch (Type)
			{
				case Texture_1D: return D3D_FL9_3_REQ_TEXTURE1D_U_DIMENSION;
				case Texture_2D: return D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION;
				case Texture_3D: return D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
				case Texture_Cube: return D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION;
				default: return 0;
			}
		}

		case D3D_FEATURE_LEVEL_10_0:
		case D3D_FEATURE_LEVEL_10_1:
		{
			switch (Type)
			{
				case Texture_1D: return D3D10_REQ_TEXTURE1D_U_DIMENSION;
				case Texture_2D: return D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
				case Texture_3D: return D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
				case Texture_Cube: return D3D10_REQ_TEXTURECUBE_DIMENSION;
				default: return 0;
			}
		}

		case D3D_FEATURE_LEVEL_11_0:
		case D3D_FEATURE_LEVEL_11_1:
		{
			switch (Type)
			{
				case Texture_1D: return D3D11_REQ_TEXTURE1D_U_DIMENSION;
				case Texture_2D: return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
				case Texture_3D: return D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
				case Texture_Cube: return D3D11_REQ_TEXTURECUBE_DIMENSION;
				default: return 0;
			}
		}

		default: return 0;
	}
}
//---------------------------------------------------------------------

int CD3D11GPUDriver::CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow)
{
	Sys::COSWindow* pWnd = pWindow; //???the same as for D3D9? - pWindow ? pWindow : D3D11DrvFactory->GetFocusWindow();
	n_assert(pWnd);

	//???or destroy and recreate with new params?
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd) return ERR_CREATION_ERROR;

	UPTR BBWidth = BackBufferDesc.Width, BBHeight = BackBufferDesc.Height;
	PrepareWindowAndBackBufferSize(*pWnd, BBWidth, BBHeight);

	// If VSync, use triple buffering by default, else double //!!! + 1 if front buffer must be included!
	UPTR BackBufferCount = SwapChainDesc.BackBufferCount ?
		SwapChainDesc.BackBufferCount :
		(SwapChainDesc.Flags.Is(SwapChain_VSync) ? 2 : 1);

	DXGI_FORMAT BackBufferFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(BackBufferDesc.Format); //???GetDesktopFormat()?! //???use SRGB?
	if (BackBufferFormat == DXGI_FORMAT_B8G8R8X8_UNORM) BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

	DXGI_SWAP_CHAIN_DESC SCDesc = { 0 };

	switch (SwapChainDesc.SwapMode)
	{
		case SwapMode_CopyPersist:	SCDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL; break;
		case SwapMode_FlipPersist:
		{
			//!!!only from Win8!
			SCDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; break;

			// BufferCount to a value between 2 and 16 to prevent a performance penalty as a result of waiting
			// on DWM to release the previous presentation buffer (c) Docs
			if (BackBufferCount < 2) BackBufferCount = 2;

			// Format to DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_B8G8R8A8_UNORM, or DXGI_FORMAT_R8G8B8A8_UNORM (c) Docs
			if (BackBufferFormat != DXGI_FORMAT_R16G16B16A16_FLOAT &&
				BackBufferFormat != DXGI_FORMAT_B8G8R8A8_UNORM &&
				BackBufferFormat != DXGI_FORMAT_R8G8B8A8_UNORM)
			{
				BackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
			}
		}
		case SwapMode_CopyDiscard:
		default:					SCDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; break;
	}

	SCDesc.BufferDesc.Width = BBWidth;
	SCDesc.BufferDesc.Height = BBHeight;
	SCDesc.BufferDesc.Format = BackBufferFormat;
	SCDesc.BufferDesc.RefreshRate.Numerator = 0;
	SCDesc.BufferDesc.RefreshRate.Denominator = 0;
	SCDesc.BufferCount = n_min(BackBufferCount, DXGI_MAX_SWAP_CHAIN_BUFFERS);
	SCDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	if (BackBufferDesc.UseAsShaderInput) SCDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
	SCDesc.Windowed = TRUE; // Recommended, use SwitchToFullscreen()
	SCDesc.OutputWindow = pWnd->GetHWND();
	SCDesc.Flags = 0; //DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // Allows automatic display mode switch on wnd->fullscr

	if (SCDesc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
	{
		// It is recommended to use non-MSAA swap chain, render to MSAA RT and then resolve it to the back buffer
		//???support MSAA or enforce separate MSAA RT?
		SCDesc.SampleDesc.Count = 1;
		SCDesc.SampleDesc.Quality = 0;
	}
	else
	{
		// MSAA not supported for swap effects other than DXGI_SWAP_EFFECT_DISCARD
		SCDesc.SampleDesc.Count = 1;
		SCDesc.SampleDesc.Quality = 0;
	}

	// Starting with Direct3D 11.1, we recommend not to use CreateSwapChain anymore to create a swap chain.
	// Instead, use CreateSwapChainForHwnd (c) Docs
	//!!!can add requirement for a platform update for win 7 and use d3d11.1 + dxgi 1.2 codepath!

	IDXGIFactory1* pDXGIFactory = D3D11DrvFactory->GetDXGIFactory();
	IDXGISwapChain* pSwapChain = NULL;
	HRESULT hr = pDXGIFactory->CreateSwapChain(pD3DDevice, &SCDesc, &pSwapChain);

	// They say if the first failure was due to wrong BufferCount, DX sets it to the correct value
	if (FAILED(hr) && SCDesc.BufferCount != BackBufferCount)
		hr = pDXGIFactory->CreateSwapChain(pD3DDevice, &SCDesc, &pSwapChain);

	if (FAILED(hr))
	{
		Sys::Error("D3D11 swap chain creation error\n");
		return ERR_CREATION_ERROR;
	}

	CArray<CD3D11SwapChain>::CIterator ItSC = SwapChains.Begin();
	for (; ItSC != SwapChains.End(); ++ItSC)
		if (!ItSC->IsValid()) break;

	if (ItSC == SwapChains.End()) ItSC = SwapChains.Reserve(1);

	ItSC->pSwapChain = pSwapChain;

	if (!InitSwapChainRenderTarget(*ItSC))
	{
		ItSC->Destroy();
		return ERR_CREATION_ERROR;
	}

	ItSC->TargetWindow = pWnd;
	ItSC->LastWindowRect = pWnd->GetRect();
	ItSC->TargetDisplay = NULL;
	ItSC->Desc = SwapChainDesc;

	//DXGI_MWA_NO_WINDOW_CHANGES, DXGI_MWA_NO_ALT_ENTER
	pDXGIFactory->MakeWindowAssociation(pWnd->GetHWND(), DXGI_MWA_NO_WINDOW_CHANGES);

	pWnd->Subscribe<CD3D11GPUDriver>(CStrID("OnToggleFullscreen"), this, &CD3D11GPUDriver::OnOSWindowToggleFullscreen, &ItSC->Sub_OnToggleFullscreen);
	pWnd->Subscribe<CD3D11GPUDriver>(CStrID("OnClosing"), this, &CD3D11GPUDriver::OnOSWindowClosing, &ItSC->Sub_OnClosing);
	if (SwapChainDesc.Flags.Is(SwapChain_AutoAdjustSize))
		pWnd->Subscribe<CD3D11GPUDriver>(CStrID("OnSizeChanged"), this, &CD3D11GPUDriver::OnOSWindowSizeChanged, &ItSC->Sub_OnSizeChanged);

	return SwapChains.IndexOf(ItSC);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::DestroySwapChain(UPTR SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	
	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		if (CurrRT[i].GetUnsafe() == SC.BackBufferRT.GetUnsafe())
		{
			SetRenderTarget(i, NULL);
			break; // Some RT may be only in one slot at a time
		}

	SC.Destroy();

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwapChainExists(UPTR SwapChainID) const
{
	return SwapChainID < SwapChains.GetCount() && SwapChains[SwapChainID].IsValid();
}
//---------------------------------------------------------------------

//!!!call ResizeTarget to resize fullscreen or windowed to resize a target window too!
//!!!use what is written now to respond do changes!
// Does not resize an OS window, since often is called in response to an OS window resize
//???bool ResizeOSWindow?
bool CD3D11GPUDriver::ResizeSwapChain(UPTR SwapChainID, unsigned int Width, unsigned int Height)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D11SwapChain& SC = SwapChains[SwapChainID];

//!!!DBG TMP!
BOOL FScr;
SC.pSwapChain->GetFullscreenState(&FScr, NULL);
Sys::DbgOut("CD3D11GPUDriver::ResizeSwapChain(%d, %d, %d), %s\n", SwapChainID, Width, Height, FScr == TRUE ? "Full" : "Wnd");
//!!!DBG TMP!

	const CRenderTargetDesc& BackBufDesc = SC.BackBufferRT->GetDesc();

	//???Or automatically choose the 0 width and height to match the client rect for HWNDs?
	// Then can pass 0 to ResizeBuffers
	if (!Width) Width = BackBufDesc.Width;
	if (!Height) Height = BackBufDesc.Height;
	if (BackBufDesc.Width == Width && BackBufDesc.Height == Height) OK;

	//???for child window, assert that size passed is a window size?

	//???or this method must resize target? in fact, need two swap chain resizing methods?
	//one as command (ResizeTarget), one as handler (ResizeBuffers), second can be OnOSWindowSizeChanged handler

	UPTR RemovedRTIdx;
	for (RemovedRTIdx = 0; RemovedRTIdx < CurrRT.GetCount(); ++RemovedRTIdx)
		if (CurrRT[RemovedRTIdx] == SC.BackBufferRT)
		{
			CurrRT[RemovedRTIdx] = NULL;
			//!!!commit changes! pD3DImmContext->OMSetRenderTargets(0, NULL, NULL);
			break;
		}

	SC.BackBufferRT->Destroy();

	UPTR SCFlags = 0; //DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// Preserve the existing buffer count and format.
	HRESULT hr = SC.pSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, SCFlags);
	n_assert(SUCCEEDED(hr));

	if (!InitSwapChainRenderTarget(SC))
	{
		SC.Destroy();
		FAIL;
	}

	//!!update DS!

	if (RemovedRTIdx < CurrRT.GetCount()) SetRenderTarget(RemovedRTIdx, SC.BackBufferRT);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwitchToFullscreen(UPTR SwapChainID, CDisplayDriver* pDisplay, const CDisplayMode* pMode)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	if (pDisplay && AdapterID != pDisplay->GetAdapterID()) FAIL;

	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	if (SC.TargetWindow->IsChild())
	{
		n_assert2(false, "WORTH TESTING!");
		FAIL;
	}

	if (pDisplay) SC.TargetDisplay = pDisplay;
	else
	{
		IDXGIOutput* pOutput = NULL;
		if (FAILED(SC.pSwapChain->GetContainingOutput(&pOutput))) FAIL;
		SC.TargetDisplay = D3D11DrvFactory->CreateDisplayDriver(pOutput);
		if (SC.TargetDisplay.IsNullPtr())
		{
			pOutput->Release();
			FAIL;
		}
	}

	IDXGIOutput* pDXGIOutput = SC.TargetDisplay->As<CD3D11DisplayDriver>()->GetDXGIOutput();
	n_assert(pDXGIOutput);

	// If pMode is NULL, DXGI will default mode to a desktop mode if
	// DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH flag is not set and to
	// a closest mode to a window size if this flag is set. We don't set it.
	DXGI_MODE_DESC DXGIMode;
	if (pMode)
	{
		// If either Width or Height are 0, both must be 0 (c) Docs for FindClosestMatchingMode()
		DXGI_MODE_DESC RequestedDXGIMode;
		RequestedDXGIMode.Width = pMode->Height ? pMode->Width : 0;
		RequestedDXGIMode.Height = pMode->Width ? pMode->Height : 0;
		RequestedDXGIMode.Format = CD3D11DriverFactory::PixelFormatToDXGIFormat(pMode->PixelFormat);
		RequestedDXGIMode.RefreshRate.Numerator = pMode->RefreshRate.Numerator;
		RequestedDXGIMode.RefreshRate.Denominator = pMode->RefreshRate.Denominator;
		RequestedDXGIMode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		RequestedDXGIMode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

		// We don't use a requested mode directly since it may be unsupported.
		// We find closest supported mode instead of it.
		// DX 11.1: IDXGIOutput1::FindClosestMatchingMode1()
		if (FAILED(pDXGIOutput->FindClosestMatchingMode(&RequestedDXGIMode, &DXGIMode, pD3DDevice)))
		{
			SC.TargetDisplay = NULL;
			FAIL;
		}

		// NB: it is recommended to call ResizeTarget() _before_ SetFullscreenState()
		if (FAILED(SC.pSwapChain->ResizeTarget(&DXGIMode)))
		{
			SC.TargetDisplay = NULL;
			FAIL;
		}
	}

	SC.LastWindowRect = SC.TargetWindow->GetRect();

//!!!DBG TMP!
Sys::DbgOut("Before Wnd -> Full, Window flags = %ld\n", SC.TargetWindow->GetWin32Style());
//!!!DBG TMP!

	HRESULT hr = SC.pSwapChain->SetFullscreenState(TRUE, pDXGIOutput);

//!!!DBG TMP!
Sys::DbgOut("After Wnd -> Full, Window flags = %ld\n", SC.TargetWindow->GetWin32Style());
//!!!DBG TMP!

	if (FAILED(hr))
	{
		if (hr == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS) OK;
		else FAIL; //???resize back?
	}

	//???is really necessary?
	// After calling SetFullscreenState, it is advisable to call ResizeTarget again with the
	// RefreshRate member of DXGI_MODE_DESC zeroed out. This amounts to a no-operation instruction
	// in DXGI, but it can avoid issues with the refresh rate. (c) Docs
	if (pMode)
	{
		DXGIMode.RefreshRate.Numerator = 0;
		DXGIMode.RefreshRate.Denominator = 0;
		if (FAILED(SC.pSwapChain->ResizeTarget(&DXGIMode)))
		{
			SC.pSwapChain->SetFullscreenState(FALSE, NULL);
			SC.TargetDisplay = NULL;
			FAIL;
		}
	}

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwitchToWindowed(UPTR SwapChainID, const Data::CRect* pWindowRect)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	SC.TargetDisplay = NULL;

//!!!DBG TMP!
Sys::DbgOut("Before Full -> Wnd, Window flags = %ld\n", SC.TargetWindow->GetWin32Style());
//!!!DBG TMP!

	HRESULT hr = SC.pSwapChain->SetFullscreenState(FALSE, NULL);

//!!!DBG TMP!
Sys::DbgOut("After Full -> Wnd, Window flags = %ld\n", SC.TargetWindow->GetWin32Style());
//!!!DBG TMP!

	if (FAILED(hr))
	{
		if (hr == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS) OK;
		else FAIL; //???resize back?
	}

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::IsFullscreen(UPTR SwapChainID) const
{
	return SwapChainExists(SwapChainID) && SwapChains[SwapChainID].IsFullscreen();
}
//---------------------------------------------------------------------

PRenderTarget CD3D11GPUDriver::GetSwapChainRenderTarget(UPTR SwapChainID) const
{
	return SwapChainExists(SwapChainID) ? SwapChains[SwapChainID].BackBufferRT : PRenderTarget();
}
//---------------------------------------------------------------------

// NB: Present() should be called as late as possible after EndFrame()
// to improve parallelism between the GPU and the CPU
bool CD3D11GPUDriver::Present(UPTR SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	CD3D11SwapChain& SC = SwapChains[SwapChainID];
	return SUCCEEDED(SC.pSwapChain->Present(0, 0));
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::CaptureScreenshot(UPTR SwapChainID, IO::CStream& OutStream) const
{
	Sys::Error("IMPLEMENT ME!!! CD3D11GPUDriver::CaptureScreenshot()\n");
	FAIL;
}
//---------------------------------------------------------------------

//!!!!!!!!!!!!???how to handle set viewport and then set another RT?

bool CD3D11GPUDriver::SetViewport(UPTR Index, const CViewport* pViewport)
{
	if (Index >= MaxViewportCount) FAIL;

	D3D11_VIEWPORT& CurrViewport = CurrVP[Index];
	UPTR IsSetBit = (1 << Index);
	if (pViewport)
	{
		if (VPSRSetFlags.Is(IsSetBit) &&
			pViewport->Left == CurrViewport.TopLeftX &&
			pViewport->Top == CurrViewport.TopLeftY &&
			pViewport->Width == CurrViewport.Width &&
			pViewport->Height == CurrViewport.Height &&
			pViewport->MinDepth == CurrViewport.MinDepth &&
			pViewport->MaxDepth == CurrViewport.MaxDepth)
		{
			OK;
		}
		
		CurrViewport.TopLeftX = pViewport->Left;
		CurrViewport.TopLeftY = pViewport->Top;
		CurrViewport.Width = pViewport->Width;
		CurrViewport.Height = pViewport->Height;
		CurrViewport.MinDepth = pViewport->MinDepth;
		CurrViewport.MaxDepth = pViewport->MaxDepth;

		VPSRSetFlags.Set(IsSetBit);
	}
	else
	{
		Sys::Error("FILL WITH RT DEFAULTS!!!\n");
		CurrViewport.TopLeftX = pViewport->Left;
		CurrViewport.TopLeftY = pViewport->Top;
		CurrViewport.Width = pViewport->Width;
		CurrViewport.Height = pViewport->Height;
		CurrViewport.MinDepth = pViewport->MinDepth;
		CurrViewport.MaxDepth = pViewport->MaxDepth;

		VPSRSetFlags.Clear(IsSetBit);
	}

	CurrDirtyFlags.Set(GPU_Dirty_VP);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::GetViewport(UPTR Index, CViewport& OutViewport)
{
	if (Index >= MaxViewportCount || (Index > 0 && VPSRSetFlags.IsNot(1 << Index))) FAIL;

	// If default (0) viewport is not set, it is of the render target size.
	// RT might be set but not applied, in that case read values from RT.
	if (Index == 0 && VPSRSetFlags.IsNot(1 << Index) && CurrDirtyFlags.Is(GPU_Dirty_RT))
	{
		for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		{
			CD3D11RenderTarget* pRT = CurrRT[i].GetUnsafe();
			if (pRT)
			{
				OutViewport.Left = 0.f;
				OutViewport.Top = 0.f;
				OutViewport.Width = (float)pRT->GetDesc().Width;
				OutViewport.Height = (float)pRT->GetDesc().Height;
				OutViewport.MinDepth = 0.f;
				OutViewport.MaxDepth = 1.f;

				OK;
			}
		}

		FAIL;
	}

	D3D11_VIEWPORT& CurrViewport = CurrVP[Index];
	OutViewport.Left = CurrViewport.TopLeftX;
	OutViewport.Top = CurrViewport.TopLeftY;
	OutViewport.Width = CurrViewport.Width;
	OutViewport.Height = CurrViewport.Height;
	OutViewport.MinDepth = CurrViewport.MinDepth;
	OutViewport.MaxDepth = CurrViewport.MaxDepth;

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetScissorRect(UPTR Index, const Data::CRect* pScissorRect)
{
	if (Index >= MaxViewportCount) FAIL;

	RECT& CurrRect = CurrSR[Index];
	UPTR IsSetBit = (1 << (VP_OR_SR_SET_FLAG_COUNT + Index)); // Use higher half of bits for SR
	if (pScissorRect)
	{
		if (VPSRSetFlags.Is(IsSetBit) &&
			pScissorRect->X == CurrRect.left &&
			pScissorRect->Y == CurrRect.top &&
			pScissorRect->Right() == CurrRect.right &&
			pScissorRect->Bottom() == CurrRect.bottom)
		{
			OK;
		}

		CurrRect.left = pScissorRect->X;
		CurrRect.top = pScissorRect->Y;
		CurrRect.right = pScissorRect->Right();
		CurrRect.bottom = pScissorRect->Bottom();

		VPSRSetFlags.Set(IsSetBit);
	}
	else
	{
		Sys::Error("FILL WITH RT DEFAULTS!!!\n");
		CurrRect.left = -1;
		CurrRect.top = -1;
		CurrRect.right = -1;
		CurrRect.bottom = -1;

		VPSRSetFlags.Clear(IsSetBit);
	}

	CurrDirtyFlags.Set(GPU_Dirty_SR);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::GetScissorRect(UPTR Index, Data::CRect& OutScissorRect)
{
	if (Index >= MaxViewportCount || VPSRSetFlags.IsNot(1 << (VP_OR_SR_SET_FLAG_COUNT + Index))) FAIL;
	RECT& CurrRect = CurrSR[Index];
	OutScissorRect.X = CurrRect.left;
	OutScissorRect.Y = CurrRect.top;
	OutScissorRect.W = CurrRect.right - CurrRect.left;
	OutScissorRect.H = CurrRect.bottom - CurrRect.top;
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetVertexLayout(CVertexLayout* pVLayout)
{
	if (CurrVL.GetUnsafe() == pVLayout) OK;
	CurrVL = (CD3D11VertexLayout*)pVLayout;
	CurrDirtyFlags.Set(GPU_Dirty_VL);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetVertexBuffer(UPTR Index, CVertexBuffer* pVB, UPTR OffsetVertex)
{
	if (Index >= CurrVB.GetCount()) FAIL;
	if (CurrVB[Index].GetUnsafe() == pVB) OK;
	CurrVB[Index] = (CD3D11VertexBuffer*)pVB;
	CurrVBOffset[Index] = pVB ? OffsetVertex * pVB->GetVertexLayout()->GetVertexSizeInBytes() : 0;
	CurrDirtyFlags.Set(GPU_Dirty_VB);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetIndexBuffer(CIndexBuffer* pIB)
{
	if (CurrIB.GetUnsafe() == pIB) OK;
	CurrIB = (CD3D11IndexBuffer*)pIB;
	CurrDirtyFlags.Set(GPU_Dirty_IB);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetRenderState(CRenderState* pState)
{
	if (CurrRS.GetUnsafe() == pState || NewRS.GetUnsafe() == pState) OK;
	NewRS = (CD3D11RenderState*)pState;
	CurrDirtyFlags.Set(GPU_Dirty_RS);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetRenderTarget(UPTR Index, CRenderTarget* pRT)
{
	if (Index >= CurrRT.GetCount()) FAIL;
	if (CurrRT[Index].GetUnsafe() == pRT) OK;

#ifdef _DEBUG // Can't set the same RT to more than one slot
	if (pRT)
		for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
			if (CurrRT[i].GetUnsafe() == pRT) FAIL;
#endif

	CurrRT[Index] = (CD3D11RenderTarget*)pRT;
	CurrDirtyFlags.Set(GPU_Dirty_RT);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetDepthStencilBuffer(CDepthStencilBuffer* pDS)
{
	if (CurrDS.GetUnsafe() == pDS) OK;
	CurrDS = (CD3D11DepthStencilBuffer*)pDS;
	CurrDirtyFlags.Set(GPU_Dirty_DS);
	OK;
}
//---------------------------------------------------------------------

//!!!ID3D11ShaderResourceView* or PObject!
bool CD3D11GPUDriver::BindSRV(EShaderType ShaderType, UPTR SlotIndex, ID3D11ShaderResourceView* pSRV)
{
	if (!pSRV || SlotIndex >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) FAIL;

	if (MaxSRVSlotIndex < SlotIndex) MaxSRVSlotIndex = SlotIndex;
	SlotIndex |= (ShaderType << 16); // Encode shader type in a high word

	IPTR DictIdx = CurrSRV.FindIndex(SlotIndex);
	if (DictIdx != INVALID_INDEX)
	{
		ID3D11ShaderResourceView*& pCurrSRV = CurrSRV.ValueAt(DictIdx);
		if (pCurrSRV == pSRV) OK;
		pCurrSRV = pSRV;
	}
	else
	{
		if (!CurrSRV.IsInAddMode()) CurrSRV.BeginAdd();
		CurrSRV.Add(SlotIndex, pSRV);
	}

	CurrDirtyFlags.Set(GPU_Dirty_SRV);
	ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_Resources + ShaderType));
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::BindConstantBuffer(EShaderType ShaderType, HConstBuffer Handle, CConstantBuffer* pCBuffer)
{
	if (!Handle) FAIL;
	CUSMBufferMeta* pMeta = (CUSMBufferMeta*)IShaderMetadata::GetHandleData(Handle);
	if (!pMeta) FAIL;

	UPTR Index = pMeta->Register;

	if (pMeta->Type == USMBuffer_Constant)
	{
		if (Index >= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT) FAIL;

		Index += ((UPTR)ShaderType) * D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
		const CD3D11ConstantBuffer* pCurrCB = CurrCB[Index].GetUnsafe();
		if (pCurrCB == pCBuffer) OK;

		CurrCB[Index] = (CD3D11ConstantBuffer*)pCBuffer;
		CurrDirtyFlags.Set(GPU_Dirty_CB);
		ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_CBuffers + ShaderType));

		OK;
	}
	else
	{
		ID3D11ShaderResourceView* pSRV = pCBuffer ? ((CD3D11ConstantBuffer*)pCBuffer)->GetD3DSRView() : NULL;
		return BindSRV(ShaderType, pMeta->Register, pSRV);
	}
}
//---------------------------------------------------------------------

//!!!???allow arrays, just add count param?! or add index with defailt 0?
bool CD3D11GPUDriver::BindResource(EShaderType ShaderType, HResource Handle, CTexture* pResource)
{
	if (!Handle) FAIL;
	CUSMResourceMeta* pMeta = (CUSMResourceMeta*)IShaderMetadata::GetHandleData(Handle);
	if (!pMeta) FAIL;

	ID3D11ShaderResourceView* pSRV = pResource ? ((CD3D11Texture*)pResource)->GetD3DSRView() : NULL;
	return BindSRV(ShaderType, pMeta->RegisterStart, pSRV);
}
//---------------------------------------------------------------------

//!!!???allow arrays, just add count param?! or add index with defailt 0?
bool CD3D11GPUDriver::BindSampler(EShaderType ShaderType, HSampler Handle, CSampler* pSampler)
{
	if (!Handle) FAIL;
	CUSMSamplerMeta* pMeta = (CUSMSamplerMeta*)IShaderMetadata::GetHandleData(Handle);
	if (!pMeta) FAIL;

	UPTR Index = pMeta->RegisterStart;
	if (Index >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT) FAIL;

	Index += ((UPTR)ShaderType) * D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
	const CD3D11Sampler* pCurrSamp = CurrSS[Index].GetUnsafe();
	if (pCurrSamp == pSampler) OK;
	if (pCurrSamp && pSampler && pCurrSamp->GetD3DSampler() == ((const CD3D11Sampler*)pSampler)->GetD3DSampler()) OK;

	CurrSS[Index] = (CD3D11Sampler*)pSampler;
	CurrDirtyFlags.Set(GPU_Dirty_SS);
	ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_Samplers + ShaderType));

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::BeginFrame()
{
	OK;
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::EndFrame()
{
}
//---------------------------------------------------------------------

// Even if RTs and DS set are still dirty (not bound), clear operation can be performed.
void CD3D11GPUDriver::Clear(UPTR Flags, const vector4& ColorRGBA, float Depth, U8 Stencil)
{
	if (Flags & Clear_Color)
	{
		for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		{
			CD3D11RenderTarget* pRT = CurrRT[i].GetUnsafe();
			if (pRT && pRT->IsValid())
				pD3DImmContext->ClearRenderTargetView(pRT->GetD3DRTView(), ColorRGBA.v);
		}
	}

	if (CurrDS.IsValidPtr() && ((Flags & Clear_Depth) || (Flags & Clear_Stencil)))
	{
		UINT D3DFlags = 0;
		if (Flags & Clear_Depth) D3DFlags |= D3D11_CLEAR_DEPTH;

		DXGI_FORMAT Fmt = CD3D11DriverFactory::PixelFormatToDXGIFormat(CurrDS->GetDesc().Format);
		if ((Flags & Clear_Stencil) && CD3D11DriverFactory::DXGIFormatStencilBits(Fmt) > 0)
			D3DFlags |= D3D11_CLEAR_STENCIL;

		pD3DImmContext->ClearDepthStencilView(CurrDS->GetD3DDSView(), D3DFlags, Depth, Stencil);
	}
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA)
{
	if (!RT.IsValid()) return;
	CD3D11RenderTarget& D3D11RT = (CD3D11RenderTarget&)RT;
	pD3DImmContext->ClearRenderTargetView(D3D11RT.GetD3DRTView(), ColorRGBA.v);
}
//---------------------------------------------------------------------

//???instance count as param?
bool CD3D11GPUDriver::Draw(const CPrimitiveGroup& PrimGroup)
{
	n_assert_dbg(pD3DDevice); // && IsInsideFrame);

	//!!!not used by Draw(), statistics only! wrap with _DEBUG/DEM_STATS!
	UPTR PrimCount = (PrimGroup.IndexCount > 0) ? PrimGroup.IndexCount : PrimGroup.VertexCount;
	if (CurrPT != PrimGroup.Topology)
	{
		D3D11_PRIMITIVE_TOPOLOGY D3DPrimType;
		switch (PrimGroup.Topology)
		{
			case Prim_PointList:	D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
			case Prim_LineList:		D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; PrimCount >>= 1; break;
			case Prim_LineStrip:	D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; --PrimCount; break;
			case Prim_TriList:		D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; PrimCount /= 3; break;
			case Prim_TriStrip:		D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; PrimCount -= 2; break;
			default:				Sys::Error("CD3D11GPUDriver::Draw() -> Invalid primitive topology!"); FAIL;
		}

		pD3DImmContext->IASetPrimitiveTopology(D3DPrimType);
		CurrPT = PrimGroup.Topology;
	}
	else
	{
		switch (CurrPT)
		{
			case Prim_PointList:	break;
			case Prim_LineList:		PrimCount >>= 1; break;
			case Prim_LineStrip:	--PrimCount; break;
			case Prim_TriList:		PrimCount /= 3; break;
			case Prim_TriStrip:		PrimCount -= 2; break;
			default:				Sys::Error("CD3D11GPUDriver::Draw() -> Invalid primitive topology!"); FAIL;
		}
	}

	if (CurrDirtyFlags.IsAny() && ApplyChanges(CurrDirtyFlags.GetMask()) != 0) FAIL;
	n_assert_dbg(CurrDirtyFlags.IsNotAll());

	if (PrimGroup.IndexCount > 0)
	{
		//!!!if InstanceCount < 2!
//StartIndexLocation [in]
//Type: UPTR
//The location of the first index read by the GPU from the index buffer.
//BaseVertexLocation [in]
//Type: INT
//A value added to each index before reading a vertex from the vertex buffer.
		pD3DImmContext->DrawIndexed(PrimGroup.IndexCount, PrimGroup.FirstIndex, PrimGroup.FirstVertex);
	}
	else
	{
		//!!!if InstanceCount < 2!
//StartVertexLocation [in]
//Type: UPTR
//Index of the first vertex, which is usually an offset in a vertex buffer.
		pD3DImmContext->Draw(PrimGroup.VertexCount, PrimGroup.FirstVertex);
	}

	//PrimsRendered += InstanceCount ? InstanceCount * PrimCount : PrimCount;
	//++DIPsRendered;

	OK;
}
//---------------------------------------------------------------------

UPTR CD3D11GPUDriver::ApplyChanges(UPTR ChangesToUpdate)
{
	Data::CFlags Update(ChangesToUpdate);
	UPTR Errors = 0;

	bool InputLayoutDirty = false;
	if (Update.Is(GPU_Dirty_RS) && CurrDirtyFlags.Is(GPU_Dirty_RS))
	{
		CD3D11RenderState* pNewRS = NewRS.GetUnsafe();

		if (pNewRS)
		{
			CD3D11RenderState* pCurrRS = CurrRS.GetUnsafe();

			UPTR CurrSigID = pCurrRS && pCurrRS->VS.IsValidPtr() ? pCurrRS->VS.GetUnsafe()->InputSignatureID : 0;
			UPTR NewSigID = pNewRS->VS.IsValidPtr() ? pNewRS->VS.GetUnsafe()->InputSignatureID : 0;

			if (!pCurrRS || NewSigID != CurrSigID) InputLayoutDirty = true;

			if (!pCurrRS || pNewRS->VS != pCurrRS->VS)
				pD3DImmContext->VSSetShader(pNewRS->VS.IsValidPtr() ? pNewRS->VS.GetUnsafe()->GetD3DVertexShader() : NULL, NULL, 0);
			if (!pCurrRS || pNewRS->HS != pCurrRS->HS)
				pD3DImmContext->HSSetShader(pNewRS->HS.IsValidPtr() ? pNewRS->HS.GetUnsafe()->GetD3DHullShader() : NULL, NULL, 0);
			if (!pCurrRS || pNewRS->DS != pCurrRS->DS)
				pD3DImmContext->DSSetShader(pNewRS->DS.IsValidPtr() ? pNewRS->DS.GetUnsafe()->GetD3DDomainShader() : NULL, NULL, 0);
			if (!pCurrRS || pNewRS->GS != pCurrRS->GS)
				pD3DImmContext->GSSetShader(pNewRS->GS.IsValidPtr() ? pNewRS->GS.GetUnsafe()->GetD3DGeometryShader() : NULL, NULL, 0);
			if (!pCurrRS || pNewRS->PS != pCurrRS->PS)
				pD3DImmContext->PSSetShader(pNewRS->PS.IsValidPtr() ? pNewRS->PS.GetUnsafe()->GetD3DPixelShader() : NULL, NULL, 0);

			if (!pCurrRS ||
				pNewRS->pBState != pCurrRS->pBState ||
				pNewRS->BlendFactorRGBA[0] != pCurrRS->BlendFactorRGBA[0] ||
				pNewRS->BlendFactorRGBA[1] != pCurrRS->BlendFactorRGBA[1] ||
				pNewRS->BlendFactorRGBA[2] != pCurrRS->BlendFactorRGBA[2] ||
				pNewRS->BlendFactorRGBA[3] != pCurrRS->BlendFactorRGBA[3] ||
				pNewRS->SampleMask != pCurrRS->SampleMask)
			{
				pD3DImmContext->OMSetBlendState(pNewRS->pBState, pNewRS->BlendFactorRGBA, pNewRS->SampleMask);
			}

			if (!pCurrRS ||
				pNewRS->pDSState != pCurrRS->pDSState ||
				pNewRS->StencilRef != pCurrRS->StencilRef)
			{
				pD3DImmContext->OMSetDepthStencilState(pNewRS->pDSState, pNewRS->StencilRef);
			}

			if (!pCurrRS || pNewRS->pRState != pCurrRS->pRState) pD3DImmContext->RSSetState(pNewRS->pRState);
		}
		else
		{
			const float EmptyFloats[4] = { 0 };
			pD3DImmContext->VSSetShader(NULL, NULL, 0);
			pD3DImmContext->HSSetShader(NULL, NULL, 0);
			pD3DImmContext->DSSetShader(NULL, NULL, 0);
			pD3DImmContext->GSSetShader(NULL, NULL, 0);
			pD3DImmContext->PSSetShader(NULL, NULL, 0);
			pD3DImmContext->OMSetBlendState(NULL, EmptyFloats, 0xffffffff);
			pD3DImmContext->OMSetDepthStencilState(NULL, 0);
			pD3DImmContext->RSSetState(NULL);
		}

		CurrRS = NewRS;
		CurrDirtyFlags.Clear(GPU_Dirty_RS);
	}

	if (Update.Is(GPU_Dirty_VL) && CurrDirtyFlags.Is(GPU_Dirty_VL))
	{
		InputLayoutDirty = true;
		CurrDirtyFlags.Clear(GPU_Dirty_VL);
	}

	if (InputLayoutDirty)
	{
		UPTR InputSigID = CurrRS.IsValidPtr() && CurrRS->VS.IsValidPtr() ? CurrRS->VS->InputSignatureID : 0;
		ID3D11InputLayout* pNewCurrIL = GetD3DInputLayout(*CurrVL, InputSigID);
		if (pCurrIL != pNewCurrIL)
		{
			pD3DImmContext->IASetInputLayout(pNewCurrIL);
			pCurrIL = pNewCurrIL;
		}
	}

	if (Update.Is(GPU_Dirty_CB) && CurrDirtyFlags.Is(GPU_Dirty_CB))
	{
		ID3D11Buffer* D3DBuffers[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
		UPTR Offset = 0;
		for (UPTR Sh = 0; Sh < ShaderType_COUNT; ++Sh, Offset += D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT)
		{
			UPTR ShdDirtyFlag = (1 << (Shader_Dirty_CBuffers + Sh));
			if (ShaderParamsDirtyFlags.IsNot(ShdDirtyFlag)) continue;

			for (UPTR i = 0; i < D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT; ++i)
			{
				const CD3D11ConstantBuffer* pCB = CurrCB[Offset + i].GetUnsafe();
				D3DBuffers[i] = pCB ? pCB->GetD3DBuffer() : NULL;
			}

			switch ((EShaderType)Sh)
			{
				case ShaderType_Vertex:
					pD3DImmContext->VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, D3DBuffers);
					break;
				case ShaderType_Pixel:
					pD3DImmContext->PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, D3DBuffers);
					break;
				case ShaderType_Geometry:
					pD3DImmContext->GSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, D3DBuffers);
					break;
				case ShaderType_Hull:
					pD3DImmContext->HSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, D3DBuffers);
					break;
				case ShaderType_Domain:
					pD3DImmContext->DSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, D3DBuffers);
					break;
			};

			ShaderParamsDirtyFlags.Clear(ShdDirtyFlag);
		}

		CurrDirtyFlags.Clear(GPU_Dirty_CB);
	}

	if (Update.Is(GPU_Dirty_SS) && CurrDirtyFlags.Is(GPU_Dirty_SS))
	{
		ID3D11SamplerState* D3DSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
		UPTR Offset = 0;
		for (UPTR Sh = 0; Sh < ShaderType_COUNT; ++Sh, Offset += D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT)
		{
			UPTR ShdDirtyFlag = (1 << (Shader_Dirty_Samplers + Sh));
			if (ShaderParamsDirtyFlags.IsNot(ShdDirtyFlag)) continue;

			for (UPTR i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
			{
				const CD3D11Sampler* pSamp = CurrSS[Offset + i].GetUnsafe();
				D3DSamplers[i] = pSamp ? pSamp->GetD3DSampler() : NULL;
			}

			switch ((EShaderType)Sh)
			{
				case ShaderType_Vertex:
					pD3DImmContext->VSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, D3DSamplers);
					break;
				case ShaderType_Pixel:
					pD3DImmContext->PSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, D3DSamplers);
					break;
				case ShaderType_Geometry:
					pD3DImmContext->GSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, D3DSamplers);
					break;
				case ShaderType_Hull:
					pD3DImmContext->HSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, D3DSamplers);
					break;
				case ShaderType_Domain:
					pD3DImmContext->DSSetSamplers(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, D3DSamplers);
					break;
			};

			ShaderParamsDirtyFlags.Clear(ShdDirtyFlag);
		}

		CurrDirtyFlags.Clear(GPU_Dirty_SS);
	}

	if (Update.Is(GPU_Dirty_SRV) && CurrDirtyFlags.Is(GPU_Dirty_SRV))
	{
		if (CurrSRV.IsInAddMode()) CurrSRV.EndAdd();

		if (CurrSRV.GetCount())
		{
			const UPTR SRVArrayMemSize = (MaxSRVSlotIndex + 1) * sizeof(ID3D11ShaderResourceView*);
			ID3D11ShaderResourceView** ppSRV = (ID3D11ShaderResourceView**)_malloca(SRVArrayMemSize);

			UPTR CurrShaderType = ShaderType_Invalid;
			UPTR ShdDirtyFlag = 0;
			bool SkipShaderType = false;
			UPTR FirstSRVSlot;
			UPTR CurrSRVSlot;
			for (UPTR i = 0; ; ++i)
			{
				UPTR ShaderType;
				UPTR SRVSlot;
				bool AtTheEnd = (i >= CurrSRV.GetCount());
				if (!AtTheEnd)
				{
					const UPTR CurrKey = CurrSRV.KeyAt(i);
					ShaderType = (CurrKey >> 16);
					SRVSlot = (CurrKey & 0x0000ffff);
				}

				if (AtTheEnd || ShaderType != CurrShaderType)
				{
					if (!SkipShaderType)
					{
						switch ((EShaderType)CurrShaderType)
						{
							case ShaderType_Vertex:
								pD3DImmContext->VSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								ShaderParamsDirtyFlags.Clear(ShdDirtyFlag);
								break;
							case ShaderType_Pixel:
								pD3DImmContext->PSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								ShaderParamsDirtyFlags.Clear(ShdDirtyFlag);
								break;
							case ShaderType_Geometry:
								pD3DImmContext->GSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								ShaderParamsDirtyFlags.Clear(ShdDirtyFlag);
								break;
							case ShaderType_Hull:
								pD3DImmContext->HSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								ShaderParamsDirtyFlags.Clear(ShdDirtyFlag);
								break;
							case ShaderType_Domain:
								pD3DImmContext->DSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								ShaderParamsDirtyFlags.Clear(ShdDirtyFlag);
								break;
						};
					}

					if (AtTheEnd) break;

					ZeroMemory(ppSRV, SRVArrayMemSize);

					CurrShaderType = ShaderType;
					FirstSRVSlot = SRVSlot;

					ShdDirtyFlag = (1 << (Shader_Dirty_Resources + CurrShaderType));
					SkipShaderType = ShaderParamsDirtyFlags.IsNot(ShdDirtyFlag);
				}

				if (SkipShaderType) continue;

				//const CD3D11Texture* pTex = CurrSRV.ValueAt(i).GetUnsafe();
				//ppSRV[SRVSlot] = pTex ? pTex->GetD3DSRView() : NULL;
				ppSRV[SRVSlot] = CurrSRV.ValueAt(i);
				CurrSRVSlot = SRVSlot;
			}

			_freea(ppSRV);
		}

		CurrDirtyFlags.Clear(GPU_Dirty_SRV);
	}

	if (Update.Is(GPU_Dirty_VB) && CurrDirtyFlags.Is(GPU_Dirty_VB))
	{
		const UPTR MaxVBCount = CurrVB.GetCount();
		const UPTR PtrsSize = sizeof(ID3D11Buffer*) * MaxVBCount;
		const UPTR UINTsSize = sizeof(UINT) * MaxVBCount;
		char* pMem = (char*)_malloca(PtrsSize + UINTsSize + UINTsSize);
		n_assert(pMem);

		ID3D11Buffer** pVBs = (ID3D11Buffer**)pMem;
		UINT* pStrides = (UINT*)(pMem + PtrsSize);
		UINT* pOffsets = (UINT*)(pMem + PtrsSize + UINTsSize);

		//???PERF: skip all NULL buffers prior to the first non-NULL and all NULL after the last non-NULL and reduce count?
		for (UPTR i = 0; i < MaxVBCount; ++i)
		{
			CD3D11VertexBuffer* pVB = CurrVB[i].GetUnsafe();
			if (pVB)
			{
				pVBs[i] = pVB->GetD3DBuffer();
				pStrides[i] = (UINT)pVB->GetVertexLayout()->GetVertexSizeInBytes();
				pOffsets[i] = (UINT)CurrVBOffset[i];
			}
			else
			{
				pVBs[i] = NULL;
				pStrides[i] = 0;
				pOffsets[i] = 0;
			}
		}

		pD3DImmContext->IASetVertexBuffers(0, MaxVBCount, pVBs, pStrides, pOffsets);

		_freea(pMem);

		CurrDirtyFlags.Clear(GPU_Dirty_VB);
	}

	if (Update.Is(GPU_Dirty_IB) && CurrDirtyFlags.Is(GPU_Dirty_IB))
	{
		if (CurrIB.IsValidPtr())
		{
			const DXGI_FORMAT Fmt = CurrIB.GetUnsafe()->GetIndexType() == Index_32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			pD3DImmContext->IASetIndexBuffer(CurrIB.GetUnsafe()->GetD3DBuffer(), Fmt, 0);
		}
		else pD3DImmContext->IASetIndexBuffer(NULL, DXGI_FORMAT_UNKNOWN, 0);

		CurrDirtyFlags.Clear(GPU_Dirty_IB);
	}

	CD3D11RenderTarget* pValidRT = NULL;

	// All render targets and a depth-stencil buffer are set atomically in D3D11
	if (Update.IsAny(GPU_Dirty_RT | GPU_Dirty_DS) && CurrDirtyFlags.IsAny(GPU_Dirty_RT | GPU_Dirty_DS))
	{
		ID3D11RenderTargetView** pRTV = (ID3D11RenderTargetView**)_malloca(sizeof(ID3D11RenderTargetView*) * CurrRT.GetCount());
		n_assert(pRTV);
		for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		{
			CD3D11RenderTarget* pRT = CurrRT[i].GetUnsafe();
			if (pRT)
			{
				pRTV[i] = pRT->GetD3DRTView();
				pValidRT = pRT;
			}
			else pRTV[i] = NULL;
		}

		ID3D11DepthStencilView* pDSV = CurrDS.IsValidPtr() ? CurrDS->GetD3DDSView() : NULL;

		pD3DImmContext->OMSetRenderTargets(CurrRT.GetCount(), pRTV, pDSV);
		//OMSetRenderTargetsAndUnorderedAccessViews

		_freea(pRTV);

		// If at least one valid RT and at least one default (unset) VP exist, we check
		// if RT dimensions are changed, and refill all default (unset) VPs properly.
		// If no valid RTs are set, we cancel VP updating as it has no meaning.
		if (pValidRT)
		{
			UPTR RTWidth = pValidRT->GetDesc().Width;
			UPTR RTHeight = pValidRT->GetDesc().Height;
			bool UnsetVPFound = false;
			for (UPTR i = 0; i < MaxViewportCount; ++i)
			{
				if (VPSRSetFlags.Is(1 << i)) continue;
				
				D3D11_VIEWPORT& D3DVP = CurrVP[i];
				if (!UnsetVPFound)
				{
					// All other default values are fixed: X = 0, Y = 0, MinDepth = 0.f, MaxDepth = 1.f
					if ((UPTR)D3DVP.Width == RTWidth && (UPTR)D3DVP.Height == RTHeight) break;
					UnsetVPFound = true;
					CurrDirtyFlags.Set(GPU_Dirty_VP);
					Update.Set(GPU_Dirty_VP);
				}

				// If we are here, RT dimensions were changed, so update default VP values
				D3DVP.Width = (float)RTWidth;
				D3DVP.Height = (float)RTHeight;
			}
		}
		else CurrDirtyFlags.Clear(GPU_Dirty_VP);

		CurrDirtyFlags.Clear(GPU_Dirty_RT | GPU_Dirty_DS);
	}

	if (Update.Is(GPU_Dirty_VP) && CurrDirtyFlags.Is(GPU_Dirty_VP))
	{
		// Find the last set VP, as we must set all viewports from 0'th to it.
		// At least one default viewport must be specified, so NumVP is no less than 1.
		UPTR NumVP = MaxViewportCount;
		for (; NumVP > 1; --NumVP)
			if (VPSRSetFlags.Is(1 << (NumVP - 1))) break;

		pD3DImmContext->RSSetViewports(NumVP, CurrVP);

		CurrDirtyFlags.Clear(GPU_Dirty_VP);
	}

	//???set viewports and scissor rects atomically?
	//???what hapens with set SRs when VPs are set?
	if (Update.Is(GPU_Dirty_SR) && CurrDirtyFlags.Is(GPU_Dirty_SR))
	{
		// Find the last set SR, as we must set all rects from 0'th to it
		UPTR NumSR = 0;
		for (; NumSR > 1; --NumSR)
			if (VPSRSetFlags.Is(1 << (VP_OR_SR_SET_FLAG_COUNT + NumSR - 1))) break;

		pD3DImmContext->RSSetScissorRects(NumSR, NumSR ? CurrSR : NULL);

		CurrDirtyFlags.Clear(GPU_Dirty_SR);
	}

	return Errors;
}
//---------------------------------------------------------------------

// Gets or creates an actual layout for the given vertex layout and shader input signature
ID3D11InputLayout* CD3D11GPUDriver::GetD3DInputLayout(CD3D11VertexLayout& VertexLayout, UPTR ShaderInputSignatureID, const Data::CBuffer* pSignature)
{
	if (!ShaderInputSignatureID) return NULL;

	ID3D11InputLayout* pLayout = VertexLayout.GetD3DInputLayout(ShaderInputSignatureID);
	if (pLayout) return pLayout;

	const D3D11_INPUT_ELEMENT_DESC* pD3DDesc = VertexLayout.GetCachedD3DLayoutDesc();
	if (!pD3DDesc) return NULL;

	const void* pData;
	UPTR Size;
	if (pSignature)
	{
		pData = pSignature->GetPtr();
		Size = pSignature->GetSize();
	}
	else
	{
		CBinaryData Binary;
		if (!D3D11DrvFactory->FindShaderInputSignature(ShaderInputSignatureID, &Binary)) FAIL;
		pData = Binary.pData;
		Size = Binary.Size;
	}

	if (FAILED(pD3DDevice->CreateInputLayout(pD3DDesc, VertexLayout.GetComponentCount(), pData, Size, &pLayout))) return NULL;

	n_verify_dbg(VertexLayout.AddLayoutObject(ShaderInputSignatureID, pLayout));

	return pLayout;
}
//---------------------------------------------------------------------

// Creates transparent user object
PVertexLayout CD3D11GPUDriver::CreateVertexLayout(const CVertexComponent* pComponents, UPTR Count)
{
	const UPTR MAX_VERTEX_COMPONENTS = 32;

	if (!pComponents || !Count || Count > MAX_VERTEX_COMPONENTS) return NULL;

	CStrID Signature = CVertexLayout::BuildSignature(pComponents, Count);

	IPTR Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx).GetUnsafe();

	UPTR MaxVertexStreams = GetMaxVertexStreams();

	D3D11_INPUT_ELEMENT_DESC DeclData[MAX_VERTEX_COMPONENTS] = { 0 };
	for (UPTR i = 0; i < Count; ++i)
	{
		const CVertexComponent& Component = pComponents[i];

		UPTR StreamIndex = Component.Stream;
		if (StreamIndex >= MaxVertexStreams) return NULL;

		D3D11_INPUT_ELEMENT_DESC& DeclElement = DeclData[i];

		switch (Component.Semantic)
		{
			case VCSem_Position:	DeclElement.SemanticName = "POSITION"; break;
			case VCSem_Normal:		DeclElement.SemanticName = "NORMAL"; break;
			case VCSem_Tangent:		DeclElement.SemanticName = "TANGENT"; break;
			case VCSem_Bitangent:	DeclElement.SemanticName = "BINORMAL"; break;
			case VCSem_TexCoord:	DeclElement.SemanticName = "TEXCOORD"; break;
			case VCSem_Color:		DeclElement.SemanticName = "COLOR"; break;
			case VCSem_BoneWeights:	DeclElement.SemanticName = "BLENDWEIGHT"; break;
			case VCSem_BoneIndices:	DeclElement.SemanticName = "BLENDINDICES"; break;
			case VCSem_UserDefined:	DeclElement.SemanticName = Component.UserDefinedName; break;
			default:				return NULL;
		}

		DeclElement.SemanticIndex = Component.Index;

		switch (Component.Format)
		{
			case VCFmt_Float32_1:		DeclElement.Format = DXGI_FORMAT_R32_FLOAT; break;
			case VCFmt_Float32_2:		DeclElement.Format = DXGI_FORMAT_R32G32_FLOAT; break;
			case VCFmt_Float32_3:		DeclElement.Format = DXGI_FORMAT_R32G32B32_FLOAT; break;
			case VCFmt_Float32_4:		DeclElement.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
			case VCFmt_Float16_2:		DeclElement.Format = DXGI_FORMAT_R16G16_FLOAT; break;
			case VCFmt_Float16_4:		DeclElement.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
			case VCFmt_UInt8_4:			DeclElement.Format = DXGI_FORMAT_R8G8B8A8_UINT; break;
			case VCFmt_UInt8_4_Norm:	DeclElement.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
			case VCFmt_SInt16_2:		DeclElement.Format = DXGI_FORMAT_R16G16_SINT; break;
			case VCFmt_SInt16_4:		DeclElement.Format = DXGI_FORMAT_R16G16B16A16_SINT; break;
			case VCFmt_SInt16_2_Norm:	DeclElement.Format = DXGI_FORMAT_R16G16_SNORM; break;
			case VCFmt_SInt16_4_Norm:	DeclElement.Format = DXGI_FORMAT_R16G16B16A16_SNORM; break;
			case VCFmt_UInt16_2_Norm:	DeclElement.Format = DXGI_FORMAT_R16G16_UNORM; break;
			case VCFmt_UInt16_4_Norm:	DeclElement.Format = DXGI_FORMAT_R16G16B16A16_UNORM; break;
			default:					return NULL;
		}

		DeclElement.InputSlot = StreamIndex;
		DeclElement.AlignedByteOffset =
			(Component.OffsetInVertex == DEM_VERTEX_COMPONENT_OFFSET_DEFAULT) ? D3D11_APPEND_ALIGNED_ELEMENT : Component.OffsetInVertex;
		DeclElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		DeclElement.InstanceDataStepRate = 0;
	}

	PD3D11VertexLayout Layout = n_new(CD3D11VertexLayout);
	if (!Layout->Create(pComponents, Count, DeclData)) return NULL;

	VertexLayouts.Add(Signature, Layout);

	return Layout.GetUnsafe();
}
//---------------------------------------------------------------------

PVertexBuffer CD3D11GPUDriver::CreateVertexBuffer(CVertexLayout& VertexLayout, UPTR VertexCount, UPTR AccessFlags, const void* pData)
{
	if (!pD3DDevice || !VertexCount || !VertexLayout.GetVertexSizeInBytes()) return NULL;

	D3D11_USAGE Usage; // GetUsageAccess() never returns immutable usage if data is not provided
	UINT CPUAccess;
	GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess);

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = Usage;
	Desc.ByteWidth = VertexCount * VertexLayout.GetVertexSizeInBytes();
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Desc.CPUAccessFlags = CPUAccess;
	Desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA* pInitData = NULL;
	D3D11_SUBRESOURCE_DATA InitData;
	if (pData)
	{
		InitData.pSysMem = pData;
		pInitData = &InitData;
	}

	ID3D11Buffer* pD3DBuf = NULL;
	if (FAILED(pD3DDevice->CreateBuffer(&Desc, pInitData, &pD3DBuf))) return NULL;

	PD3D11VertexBuffer VB = n_new(CD3D11VertexBuffer);
	if (!VB->Create(VertexLayout, pD3DBuf))
	{
		pD3DBuf->Release();
		return NULL;
	}

	return VB.GetUnsafe();
}
//---------------------------------------------------------------------

PIndexBuffer CD3D11GPUDriver::CreateIndexBuffer(EIndexType IndexType, UPTR IndexCount, UPTR AccessFlags, const void* pData)
{
	if (!pD3DDevice || !IndexCount) return NULL;

	D3D11_USAGE Usage; // GetUsageAccess() never returns immutable usage if data is not provided
	UINT CPUAccess;
	GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess);

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = Usage;
	Desc.ByteWidth = IndexCount * (UPTR)IndexType;
	Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	Desc.CPUAccessFlags = CPUAccess;
	Desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA* pInitData = NULL;
	D3D11_SUBRESOURCE_DATA InitData;
	if (pData)
	{
		InitData.pSysMem = pData;
		pInitData = &InitData;
	}

	ID3D11Buffer* pD3DBuf = NULL;
	if (FAILED(pD3DDevice->CreateBuffer(&Desc, pInitData, &pD3DBuf))) return NULL;

	PD3D11IndexBuffer IB = n_new(CD3D11IndexBuffer);
	if (!IB->Create(IndexType, pD3DBuf))
	{
		pD3DBuf->Release();
		return NULL;
	}

	return IB.GetUnsafe();
}
//---------------------------------------------------------------------

//!!!shader reflection doesn't return StructuredBuffer element count! So we must pass it here and ignore parameter for other buffers!
//!!!or we must determine buffer size in shader comments(annotations?) / in effect desc!
PConstantBuffer CD3D11GPUDriver::CreateConstantBuffer(HConstBuffer hBuffer, UPTR AccessFlags, const CConstantBuffer* pData)
{
	if (!pD3DDevice || !hBuffer) return NULL;
	CUSMBufferMeta* pMeta = (CUSMBufferMeta*)IShaderMetadata::GetHandleData(hBuffer);
	if (!pMeta) return NULL;

	D3D11_USAGE Usage; // GetUsageAccess() never returns immutable usage if data is not provided
	UINT CPUAccess;
	GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess);

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CPUAccess;

	UPTR TotalSize = pMeta->Size; // * ElementCount; //!!!for StructuredBuffer!
	if (pMeta->Type == USMBuffer_Constant)
	{
		UPTR ElementCount = (TotalSize + 15) >> 4;
		if (ElementCount > D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT) return NULL;
		Desc.ByteWidth = ElementCount << 4;
		Desc.BindFlags = (Usage == D3D11_USAGE_STAGING) ? 0 : D3D11_BIND_CONSTANT_BUFFER;
	}
	else
	{
		Desc.ByteWidth = TotalSize;
		Desc.BindFlags = (Usage == D3D11_USAGE_STAGING) ? 0 : D3D11_BIND_SHADER_RESOURCE;
	}

	if (pMeta->Type == USMBuffer_Structured)
	{
		Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		Desc.StructureByteStride = pMeta->Size;
	}
	else
	{
		Desc.MiscFlags = 0;
		Desc.StructureByteStride = 0;
	}

	D3D11_SUBRESOURCE_DATA* pInitData = NULL;
	D3D11_SUBRESOURCE_DATA InitData;
	if (pData)
	{
		if (!pData->IsA<CD3D11ConstantBuffer>()) FAIL;
		const CD3D11ConstantBuffer* pDataCB11 = (const CD3D11ConstantBuffer*)pData;
		InitData.pSysMem = pDataCB11->GetRAMCopy();
		pInitData = &InitData;
	}

	ID3D11Buffer* pD3DBuf = NULL;
	if (FAILED(pD3DDevice->CreateBuffer(&Desc, pInitData, &pD3DBuf))) return NULL;

	ID3D11ShaderResourceView* pSRV = NULL;
	if (Desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		if (FAILED(pD3DDevice->CreateShaderResourceView(pD3DBuf, NULL, &pSRV)))
		{
			pD3DBuf->Release();
			return NULL;
		}
	}

	PD3D11ConstantBuffer CB = n_new(CD3D11ConstantBuffer);
	if (!CB->Create(pD3DBuf, pSRV))
	{
		if (pSRV) pSRV->Release();
		pD3DBuf->Release();
		return NULL;
	}

	//???or add manual control / some flag in a metadata?
	if (Usage != D3D11_USAGE_IMMUTABLE)
	{
		if (!CB->CreateRAMCopy()) return NULL;
	}

	return CB.GetUnsafe();
}
//---------------------------------------------------------------------

// pData - initial data to be uploaded to a texture.
// if MipDataProvided, order is ArrayElement[0] { Mip[0] ... Mip[N] } ... ArrayElement[M] { Mip[0] ... Mip[N] },
// else order is ArrayElement[0] { Mip[0] } ... ArrayElement[M] { Mip[0] }, where Mip[0] is an original data.
PTexture CD3D11GPUDriver::CreateTexture(const CTextureDesc& Desc, UPTR AccessFlags, const void* pData, bool MipDataProvided)
{
	if (!pD3DDevice || !Desc.Width || !Desc.Height) return NULL;
	
	if (Desc.Type != Texture_1D && Desc.Type != Texture_2D && Desc.Type != Texture_Cube && Desc.Type != Texture_3D)
	{
		Sys::Error("CD3D11GPUDriver::CreateTexture() > Unknown texture type %d\n", Desc.Type);
		return NULL;
	}

	PD3D11Texture Tex = n_new(CD3D11Texture);
	if (Tex.IsNullPtr()) return NULL;

	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);
	UPTR QualityLvlCount = 0;
	if (Desc.MSAAQuality != MSAA_None)
	{
		if (FAILED(pD3DDevice->CheckMultisampleQualityLevels(DXGIFormat, (int)Desc.MSAAQuality, &QualityLvlCount)) || !QualityLvlCount) return NULL;
		pData = NULL; // MSAA resources can't be initialized with data
	}

	D3D11_USAGE Usage; // GetUsageAccess() never returns immutable usage if data is not provided
	UINT CPUAccess;
	GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess);

	UPTR MipLevels = Desc.MipLevels;
	UPTR ArraySize = (Desc.Type == Texture_3D) ? 1 : ((Desc.Type == Texture_Cube) ? 6 * Desc.ArraySize : Desc.ArraySize);
	UPTR MiscFlags = 0; //???if (MipLevels != 1) D3D11_RESOURCE_MISC_RESOURCE_CLAMP for ID3D11DeviceContext::SetResourceMinLOD()
	UPTR BindFlags = (Usage != D3D11_USAGE_STAGING) ? D3D11_BIND_SHADER_RESOURCE : 0;

	// Dynamic SRV: The resource can only be created with a single subresource.
	// The resource cannot be a texture array. The resource cannot be a mipmap chain (c) Docs
	if (Usage == D3D11_USAGE_DYNAMIC && (MipLevels != 1 || ArraySize != 1))
	{
#ifdef _DEBUG
		Sys::DbgOut("CD3D11GPUDriver::CreateTexture() > Dynamic texture requested %d mips and %d array slices. D3D11 requires 1 for both. Values are changed to 1.\n", MipLevels, ArraySize);
#endif
		MipLevels = 1;
		ArraySize = 1;
	}

	D3D11_SUBRESOURCE_DATA* pInitData = NULL;
	if (pData)
	{
		UPTR BlockSize = CD3D11DriverFactory::DXGIFormatBlockSize(DXGIFormat);

		if (!MipLevels) MipLevels = GetMipLevelCount(Desc.Width, Desc.Height, BlockSize);
		n_assert_dbg(MipLevels <= D3D11_REQ_MIP_LEVELS);
		if (MipLevels > D3D11_REQ_MIP_LEVELS) MipLevels = D3D11_REQ_MIP_LEVELS;

		UPTR BPP = CD3D11DriverFactory::DXGIFormatBitsPerPixel(DXGIFormat);
		n_assert_dbg(BPP > 0);

		//???truncate MipCount if 1x1 size is reached before the last mip?
		//???texture and/or utility [inline] methods GetRowPitch(level, w, [h], fmt), GetSlicePitch(level, w, h, fmt OR rowpitch, h)?
		UPTR Pitch[D3D11_REQ_MIP_LEVELS], SlicePitch[D3D11_REQ_MIP_LEVELS];
		if (BlockSize == 1)
		{
			for (UPTR Mip = 0; Mip < MipLevels; ++Mip)
			{
				UPTR MipWidth = Desc.Width >> Mip;
				UPTR MipHeight = Desc.Height >> Mip;
				if (!MipWidth) MipWidth = 1;
				if (!MipHeight) MipHeight = 1;
				Pitch[Mip] = (MipWidth * BPP + 7) >> 3; // Round up to the nearest byte
				SlicePitch[Mip] = Pitch[Mip] * MipHeight;
			}
		}
		else
		{
			for (UPTR Mip = 0; Mip < MipLevels; ++Mip)
			{
				UPTR MipWidth = Desc.Width >> Mip;
				UPTR MipHeight = Desc.Height >> Mip;
				if (!MipWidth) MipWidth = 1;
				if (!MipHeight) MipHeight = 1;
				UPTR BlockCountW = (MipWidth + BlockSize - 1) / BlockSize;
				UPTR BlockCountH = (MipHeight + BlockSize - 1) / BlockSize;
				Pitch[Mip] = (BlockCountW * BlockSize * BlockSize * BPP) >> 3;
				SlicePitch[Mip] = Pitch[Mip] * BlockCountH;
			}
		}

		// GPU mipmap generation is supported only for SRV + RT textures and is intended for
		// generating mips for texture render targets. Static mips are either pregenerated by a
		// toolchain or are omitted to save HDD loading time and will be generated now on CPU.
		// Order is ArrayElement[0] { Mip[1] ... Mip[N] } ... ArrayElement[M] { Mip[1] ... Mip[N] }.
		// Most detailed data Mip[0] is not copied to this buffer. Can also generate mips async in loader.
		char* pGeneratedMips = NULL;
		if (!MipDataProvided && MipLevels > 1)
		{
			Sys::Error("IMPLEMENT ME!\n");
			//!!!generate mips by DirectXTex code or smth like that!
			//!!!take this into account when setting pointers below!
#ifdef _DEBUG
			if (!pGeneratedMips)
				Sys::DbgOut("CD3D11GPUDriver::CreateTexture() > Mipmaps are not generated\n");
#endif
		}

		pInitData = (D3D11_SUBRESOURCE_DATA*)_malloca(MipLevels * ArraySize * sizeof(D3D11_SUBRESOURCE_DATA));
		D3D11_SUBRESOURCE_DATA* pCurrInitData = pInitData;
		U8* pCurrData = (U8*)pData;
		for (UPTR Elm = 0; Elm < ArraySize; ++Elm)
		{
			pCurrInitData->pSysMem = pCurrData;
			pCurrInitData->SysMemPitch = Pitch[0];
			pCurrInitData->SysMemSlicePitch = SlicePitch[0];
			pCurrData += SlicePitch[0]; //!!!* MipDepth!
			++pCurrInitData;

			for (UPTR Mip = 1; Mip < MipLevels; ++Mip)
			{
				if (MipDataProvided)
				{
					pCurrInitData->pSysMem = pCurrData;
					pCurrInitData->SysMemPitch = Pitch[Mip];
					pCurrInitData->SysMemSlicePitch = SlicePitch[Mip];
					pCurrData += SlicePitch[Mip]; //!!!* MipDepth!
					++pCurrInitData;
				}
				else if (pGeneratedMips)
				{
					pCurrInitData->pSysMem = pGeneratedMips;
					pCurrInitData->SysMemPitch = Pitch[Mip];
					pCurrInitData->SysMemSlicePitch = SlicePitch[Mip];
					pGeneratedMips += SlicePitch[Mip]; //!!!* MipDepth!
					++pCurrInitData;
				}
				else
				{
					pCurrInitData->pSysMem = NULL;
					pCurrInitData->SysMemPitch = 0;
					pCurrInitData->SysMemSlicePitch = 0;
					++pCurrInitData;
				}
			}
		}
	}

	ID3D11Resource* pTexRsrc = NULL;
	if (Desc.Type == Texture_1D)
	{
		D3D11_TEXTURE1D_DESC D3DDesc = { 0 };
		D3DDesc.Width = Desc.Width;
		D3DDesc.MipLevels = MipLevels;
		D3DDesc.ArraySize = ArraySize;
		D3DDesc.Format = DXGIFormat;
		D3DDesc.Usage = Usage;
		D3DDesc.BindFlags = BindFlags;
		D3DDesc.CPUAccessFlags = CPUAccess;	
		D3DDesc.MiscFlags = MiscFlags;

		ID3D11Texture1D* pD3DTex = NULL;
		HRESULT hr = pD3DDevice->CreateTexture1D(&D3DDesc, pInitData, &pD3DTex);
		if (pInitData) _freea(pInitData);
		if (FAILED(hr)) return NULL;
		pTexRsrc = pD3DTex;
	}
	else if (Desc.Type == Texture_2D || Desc.Type == Texture_Cube)
	{
		D3D11_TEXTURE2D_DESC D3DDesc = { 0 };
		D3DDesc.Width = Desc.Width;
		D3DDesc.Height = Desc.Height;
		D3DDesc.MipLevels = MipLevels;
		D3DDesc.ArraySize = ArraySize;
		D3DDesc.Format = DXGIFormat;
		D3DDesc.Usage = Usage;
		D3DDesc.BindFlags = BindFlags;
		D3DDesc.CPUAccessFlags = CPUAccess;	
		D3DDesc.MiscFlags = MiscFlags;

		if (Desc.Type == Texture_Cube)
			D3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

		if (Desc.MSAAQuality == MSAA_None)
		{
			D3DDesc.SampleDesc.Count = 1;
			D3DDesc.SampleDesc.Quality = 0;
		}
		else
		{
			D3DDesc.SampleDesc.Count = (int)Desc.MSAAQuality;
			D3DDesc.SampleDesc.Quality = QualityLvlCount - 1; // Can use predefined D3D11_STANDARD_MULTISAMPLE_PATTERN, D3D11_CENTER_MULTISAMPLE_PATTERN
		}

		ID3D11Texture2D* pD3DTex = NULL;
		HRESULT hr = pD3DDevice->CreateTexture2D(&D3DDesc, pInitData, &pD3DTex);
		if (pInitData) _freea(pInitData);
		if (FAILED(hr)) return NULL;
		pTexRsrc = pD3DTex;
	}
	else if (Desc.Type == Texture_3D)
	{
		D3D11_TEXTURE3D_DESC D3DDesc = { 0 };
		D3DDesc.Width = Desc.Width;
		D3DDesc.Height = Desc.Height;
		D3DDesc.Depth = Desc.Depth;
		D3DDesc.MipLevels = MipLevels;
		D3DDesc.Format = DXGIFormat;
		D3DDesc.Usage = Usage;
		D3DDesc.BindFlags = BindFlags;
		D3DDesc.CPUAccessFlags = CPUAccess;	
		D3DDesc.MiscFlags = MiscFlags;

		ID3D11Texture3D* pD3DTex = NULL;
		HRESULT hr = pD3DDevice->CreateTexture3D(&D3DDesc, pInitData, &pD3DTex);
		if (pInitData) _freea(pInitData);
		if (FAILED(hr)) return NULL;
		pTexRsrc = pD3DTex;
	}

	ID3D11ShaderResourceView* pSRV = NULL;
	if ((BindFlags & D3D11_BIND_SHADER_RESOURCE) &&
		FAILED(pD3DDevice->CreateShaderResourceView(pTexRsrc, NULL, &pSRV)))
	{
		pTexRsrc->Release();
		return NULL;
	}

	if (!Tex->Create(pTexRsrc, pSRV))
	{
		pSRV->Release();
		pTexRsrc->Release();
		return NULL;
	}

	return Tex.GetUnsafe();
}
//---------------------------------------------------------------------

PSampler CD3D11GPUDriver::CreateSampler(const CSamplerDesc& Desc)
{
	D3D11_SAMPLER_DESC D3DDesc;
	D3DDesc.AddressU = GetD3DTexAddressMode(Desc.AddressU);
	D3DDesc.AddressV = GetD3DTexAddressMode(Desc.AddressV);
	D3DDesc.AddressW = GetD3DTexAddressMode(Desc.AddressW);
	memcpy(D3DDesc.BorderColor, Desc.BorderColorRGBA, sizeof(D3DDesc.BorderColor));
	D3DDesc.ComparisonFunc = GetD3DCmpFunc(Desc.CmpFunc);
	D3DDesc.Filter = GetD3DTexFilter(Desc.Filter, (Desc.CmpFunc != Cmp_Never));
	D3DDesc.MaxAnisotropy = Clamp<unsigned int>(Desc.MaxAnisotropy, 1, 16);
	D3DDesc.MaxLOD = Desc.CoarsestMipMapLOD;
	D3DDesc.MinLOD = Desc.FinestMipMapLOD;
	D3DDesc.MipLODBias = Desc.MipMapLODBias;

	ID3D11SamplerState* pD3DSamplerState = NULL;
	if (FAILED(pD3DDevice->CreateSamplerState(&D3DDesc, &pD3DSamplerState))) return NULL;

	// Since sampler creation should be load-time, it is not performance critical.
	// We can omit it and allow to create duplicate samplers, but maintaining uniquity
	// serves both for memory saving and early exits on redundant binding.
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CD3D11Sampler* pSamp = Samplers[i].GetUnsafe();
		if (pSamp->GetD3DSampler() == pD3DSamplerState) return pSamp; //???release pD3DSamplerState?
	}

	PD3D11Sampler Samp = n_new(CD3D11Sampler);
	if (!Samp->Create(pD3DSamplerState))
	{
		pD3DSamplerState->Release();
		return NULL;
	}

	Samplers.Add(Samp);

	return Samp.GetUnsafe();
}
//---------------------------------------------------------------------

//???allow arrays? allow 3D and cubes? will need RT.Create or CreateRenderTarget(Texture, SurfaceLocation)
PRenderTarget CD3D11GPUDriver::CreateRenderTarget(const CRenderTargetDesc& Desc)
{
	DXGI_FORMAT Fmt = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);

	UPTR QualityLvlCount = 0;
	if (Desc.MSAAQuality != MSAA_None)
		if (FAILED(pD3DDevice->CheckMultisampleQualityLevels(Fmt, (int)Desc.MSAAQuality, &QualityLvlCount)) || !QualityLvlCount) return NULL;

	D3D11_TEXTURE2D_DESC D3DDesc = {0};
	D3DDesc.Width = Desc.Width;
	D3DDesc.Height = Desc.Height;
	D3DDesc.MipLevels = Desc.UseAsShaderInput ? Desc.MipLevels : 1;
	D3DDesc.ArraySize = 1;
	D3DDesc.Format = Fmt;
	if (Desc.MSAAQuality == MSAA_None)
	{
		D3DDesc.SampleDesc.Count = 1;
		D3DDesc.SampleDesc.Quality = 0;
	}
	else
	{
		D3DDesc.SampleDesc.Count = (int)Desc.MSAAQuality;
		D3DDesc.SampleDesc.Quality = QualityLvlCount - 1; // Can use predefined D3D11_STANDARD_MULTISAMPLE_PATTERN, D3D11_CENTER_MULTISAMPLE_PATTERN
	}
	D3DDesc.Usage = D3D11_USAGE_DEFAULT;
	D3DDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
	D3DDesc.CPUAccessFlags = 0;
	D3DDesc.MiscFlags = 0;
	if (Desc.UseAsShaderInput)
	{
		D3DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if (Desc.MipLevels != 1)
			D3DDesc.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS; // | D3D11_RESOURCE_MISC_RESOURCE_CLAMP
	}

	ID3D11Texture2D* pTexture = NULL;
	if (FAILED(pD3DDevice->CreateTexture2D(&D3DDesc, NULL, &pTexture))) return NULL;
	
	ID3D11RenderTargetView* pRTV = NULL;
	if (FAILED(pD3DDevice->CreateRenderTargetView(pTexture, NULL, &pRTV)))
	{
		pTexture->Release();
		return NULL;
	}

	ID3D11ShaderResourceView* pSRV = NULL;
	if (Desc.UseAsShaderInput)
	{
		if (FAILED(pD3DDevice->CreateShaderResourceView(pTexture, NULL, &pSRV)))
		{
			pRTV->Release();
			pTexture->Release();
			return NULL;
		}
	}

	pTexture->Release();

	PD3D11RenderTarget RT = n_new(CD3D11RenderTarget);
	if (!RT->Create(pRTV, pSRV))
	{
		if (pSRV) pSRV->Release();
		pRTV->Release();
		return NULL;
	}
	return RT.GetUnsafe();
}
//---------------------------------------------------------------------

PDepthStencilBuffer CD3D11GPUDriver::CreateDepthStencilBuffer(const CRenderTargetDesc& Desc)
{
	DXGI_FORMAT DSVFmt = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);
	if (DSVFmt == DXGI_FORMAT_UNKNOWN) FAIL;

	DXGI_FORMAT RsrcFmt, SRVFmt;
	if (Desc.UseAsShaderInput)
	{
		RsrcFmt = CD3D11DriverFactory::GetCorrespondingFormat(DSVFmt, FmtType_Typeless);
		if (RsrcFmt == DXGI_FORMAT_UNKNOWN) FAIL;
		SRVFmt = CD3D11DriverFactory::GetCorrespondingFormat(RsrcFmt, FmtType_Float, false);
		if (SRVFmt == DXGI_FORMAT_UNKNOWN) FAIL;
	}
	else RsrcFmt = DSVFmt;

	//???check DSV or texture fmt?
	UPTR QualityLvlCount = 0;
	if (Desc.MSAAQuality != MSAA_None)
		if (FAILED(pD3DDevice->CheckMultisampleQualityLevels(DSVFmt, (int)Desc.MSAAQuality, &QualityLvlCount)) || !QualityLvlCount) return NULL;

	D3D11_TEXTURE2D_DESC D3DDesc = {0};
	D3DDesc.Width = Desc.Width;
	D3DDesc.Height = Desc.Height;
	D3DDesc.MipLevels = 1;
	D3DDesc.ArraySize = 1;
	D3DDesc.Format = RsrcFmt;
	if (Desc.MSAAQuality == MSAA_None)
	{
		D3DDesc.SampleDesc.Count = 1;
		D3DDesc.SampleDesc.Quality = 0;
	}
	else
	{
		D3DDesc.SampleDesc.Count = (int)Desc.MSAAQuality;
		D3DDesc.SampleDesc.Quality = QualityLvlCount - 1; // Can use predefined D3D11_STANDARD_MULTISAMPLE_PATTERN, D3D11_CENTER_MULTISAMPLE_PATTERN
	}
	D3DDesc.Usage = D3D11_USAGE_DEFAULT;
	D3DDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	if (Desc.UseAsShaderInput) D3DDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
	D3DDesc.CPUAccessFlags = 0;
	D3DDesc.MiscFlags = 0;

	ID3D11Texture2D* pTexture = NULL;
	if (FAILED(pD3DDevice->CreateTexture2D(&D3DDesc, NULL, &pTexture))) return NULL;

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	if (Desc.MSAAQuality == MSAA_None)
	{
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice = 0;
	}
	else DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	DSVDesc.Format = DSVFmt;
	DSVDesc.Flags = 0; // D3D11_DSV_READ_ONLY_DEPTH, D3D11_DSV_READ_ONLY_STENCIL

	ID3D11DepthStencilView* pDSV = NULL;
	if (FAILED(pD3DDevice->CreateDepthStencilView(pTexture, &DSVDesc, &pDSV)))
	{
		pTexture->Release();
		return NULL;
	}

	ID3D11ShaderResourceView* pSRV = NULL;
	if (Desc.UseAsShaderInput)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		if (Desc.MSAAQuality == MSAA_None)
		{
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SRVDesc.Texture2D.MipLevels = -1;
			SRVDesc.Texture2D.MostDetailedMip = 0;
		}
		else SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
		SRVDesc.Format = SRVFmt;

		if (FAILED(pD3DDevice->CreateShaderResourceView(pTexture, &SRVDesc, &pSRV)))
		{
			pDSV->Release();
			pTexture->Release();
			return NULL;
		}
	}

	pTexture->Release();

	PD3D11DepthStencilBuffer DS = n_new(CD3D11DepthStencilBuffer);
	if (!DS->Create(pDSV, pSRV))
	{
		if (pSRV) pSRV->Release();
		pDSV->Release();
		return NULL;
	}
	return DS.GetUnsafe();
}
//---------------------------------------------------------------------

//???is AddRef invoked if runtime finds existing state?
//!!!can create solid and wireframe variants of the same state for the fast switching!
//???unused substates force default? say, stencil off - all stencil settings are default,
//not to duplicate states.
PRenderState CD3D11GPUDriver::CreateRenderState(const CRenderStateDesc& Desc)
{
	// Not supported (implement in D3D11 shaders):
	// - Misc_AlphaTestEnable
	// - AlphaTestRef
	// - AlphaTestFunc
	// - Misc_ClipPlaneEnable << 0 .. 5

	// Convert absolute depth bias to D3D11-style.
	// Depth buffer is always floating-point, let it be 32-bit, with 23 bits for mantissa.
	float DepthBias = Desc.DepthBias * (float)(1 << 23);

	D3D11_RASTERIZER_DESC RDesc;
	RDesc.FillMode = Desc.Flags.Is(CRenderStateDesc::Rasterizer_Wireframe) ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	const bool CullFront = Desc.Flags.Is(CRenderStateDesc::Rasterizer_CullFront);
	const bool CullBack = Desc.Flags.Is(CRenderStateDesc::Rasterizer_CullBack);
	if (!CullFront && !CullBack) RDesc.CullMode = D3D11_CULL_NONE;
	else if (CullBack) RDesc.CullMode = D3D11_CULL_BACK;
	else RDesc.CullMode = D3D11_CULL_FRONT;
	RDesc.FrontCounterClockwise = Desc.Flags.Is(CRenderStateDesc::Rasterizer_FrontCCW);
	RDesc.DepthBias = (INT)DepthBias;
	RDesc.DepthBiasClamp = Desc.DepthBiasClamp;
	RDesc.SlopeScaledDepthBias = Desc.SlopeScaledDepthBias;
	RDesc.DepthClipEnable = Desc.Flags.Is(CRenderStateDesc::Rasterizer_DepthClipEnable);
	RDesc.ScissorEnable = Desc.Flags.Is(CRenderStateDesc::Rasterizer_ScissorEnable);
	RDesc.MultisampleEnable = Desc.Flags.Is(CRenderStateDesc::Rasterizer_MSAAEnable);
	RDesc.AntialiasedLineEnable = Desc.Flags.Is(CRenderStateDesc::Rasterizer_MSAALinesEnable);

	ID3D11RasterizerState* pRState = NULL;
	if (FAILED(pD3DDevice->CreateRasterizerState(&RDesc, &pRState))) goto ProcessFailure;

	D3D11_DEPTH_STENCIL_DESC DSDesc;
	DSDesc.DepthEnable = Desc.Flags.Is(CRenderStateDesc::DS_DepthEnable);
	DSDesc.DepthWriteMask = Desc.Flags.Is(CRenderStateDesc::DS_DepthWriteEnable) ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	DSDesc.DepthFunc = GetD3DCmpFunc(Desc.DepthFunc);
	DSDesc.StencilEnable = Desc.Flags.Is(CRenderStateDesc::DS_StencilEnable);
	DSDesc.StencilReadMask = Desc.StencilReadMask;
	DSDesc.StencilWriteMask = Desc.StencilWriteMask;
	DSDesc.FrontFace.StencilFunc = GetD3DCmpFunc(Desc.StencilFrontFace.StencilFunc);
	DSDesc.FrontFace.StencilPassOp = GetD3DStencilOp(Desc.StencilFrontFace.StencilPassOp);
	DSDesc.FrontFace.StencilFailOp = GetD3DStencilOp(Desc.StencilFrontFace.StencilFailOp);
	DSDesc.FrontFace.StencilDepthFailOp = GetD3DStencilOp(Desc.StencilFrontFace.StencilDepthFailOp);
	DSDesc.BackFace.StencilFunc = GetD3DCmpFunc(Desc.StencilBackFace.StencilFunc);
	DSDesc.BackFace.StencilPassOp = GetD3DStencilOp(Desc.StencilBackFace.StencilPassOp);
	DSDesc.BackFace.StencilFailOp = GetD3DStencilOp(Desc.StencilBackFace.StencilFailOp);
	DSDesc.BackFace.StencilDepthFailOp = GetD3DStencilOp(Desc.StencilBackFace.StencilDepthFailOp);

	ID3D11DepthStencilState* pDSState = NULL;
	if (FAILED(pD3DDevice->CreateDepthStencilState(&DSDesc, &pDSState))) goto ProcessFailure;

	D3D11_BLEND_DESC BDesc;
	BDesc.IndependentBlendEnable = Desc.Flags.Is(CRenderStateDesc::Blend_Independent);
	BDesc.AlphaToCoverageEnable = Desc.Flags.Is(CRenderStateDesc::Blend_AlphaToCoverage);
	for (UPTR i = 0; i < 8; ++i)
	{
		D3D11_RENDER_TARGET_BLEND_DESC& RTDesc = BDesc.RenderTarget[i];
		if (i == 0 || BDesc.IndependentBlendEnable)
		{
			const CRenderStateDesc::CRTBlend& SrcRTDesc = Desc.RTBlend[i];
			RTDesc.BlendEnable = Desc.Flags.Is(CRenderStateDesc::Blend_RTBlendEnable << i);
			RTDesc.SrcBlend = GetD3DBlendArg(SrcRTDesc.SrcBlendArg);
			RTDesc.DestBlend = GetD3DBlendArg(SrcRTDesc.DestBlendArg);
			RTDesc.BlendOp = GetD3DBlendOp(SrcRTDesc.BlendOp);
			RTDesc.SrcBlendAlpha = GetD3DBlendArg(SrcRTDesc.SrcBlendArgAlpha);
			RTDesc.DestBlendAlpha = GetD3DBlendArg(SrcRTDesc.DestBlendArgAlpha);
			RTDesc.BlendOpAlpha = GetD3DBlendOp(SrcRTDesc.BlendOpAlpha);
			RTDesc.RenderTargetWriteMask = SrcRTDesc.WriteMask & 0x0f;
		}
		else
		{
			RTDesc.BlendEnable = FALSE;
			RTDesc.SrcBlend = D3D11_BLEND_ONE;
			RTDesc.DestBlend = D3D11_BLEND_ZERO;
			RTDesc.BlendOp = D3D11_BLEND_OP_ADD;
			RTDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
			RTDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
			RTDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			RTDesc.RenderTargetWriteMask = 0;
		}
	}

	ID3D11BlendState* pBState = NULL;
	if (FAILED(pD3DDevice->CreateBlendState(&BDesc, &pBState))) goto ProcessFailure;

	// Since render state creation should be load-time, it is not performance critical. If we
	// skip this and create new CRenderState, sorting will consider them as different state sets.
	for (UPTR i = 0; i < RenderStates.GetCount(); ++i)
	{
		CD3D11RenderState* pRS = RenderStates[i].GetUnsafe();
		if (pRS->VS == Desc.VertexShader &&
			pRS->PS == Desc.PixelShader &&
			pRS->GS == Desc.GeometryShader &&
			pRS->HS == Desc.HullShader &&
			pRS->DS == Desc.DomainShader &&
			pRS->pRState == pRState &&
			pRS->pDSState == pDSState &&
			pRS->pBState == pBState &&
			pRS->StencilRef == Desc.StencilRef &&
			pRS->BlendFactorRGBA[0] == Desc.BlendFactorRGBA[0] &&
			pRS->BlendFactorRGBA[1] == Desc.BlendFactorRGBA[1] &&
			pRS->BlendFactorRGBA[2] == Desc.BlendFactorRGBA[2] &&
			pRS->BlendFactorRGBA[3] == Desc.BlendFactorRGBA[3] &&
			pRS->SampleMask == Desc.SampleMask)
		{
			return pRS; //???release state interfaces?
		}
	}

	{
		PD3D11RenderState RS = n_new(CD3D11RenderState);
		RS->VS = (CD3D11Shader*)Desc.VertexShader.GetUnsafe();
		RS->PS = (CD3D11Shader*)Desc.PixelShader.GetUnsafe();
		RS->GS = (CD3D11Shader*)Desc.GeometryShader.GetUnsafe();
		RS->HS = (CD3D11Shader*)Desc.HullShader.GetUnsafe();
		RS->DS = (CD3D11Shader*)Desc.DomainShader.GetUnsafe();
		RS->pRState = pRState;
		RS->pDSState = pDSState;
		RS->pBState = pBState;
		RS->StencilRef = Desc.StencilRef;
		RS->BlendFactorRGBA[0] = Desc.BlendFactorRGBA[0];
		RS->BlendFactorRGBA[1] = Desc.BlendFactorRGBA[1];
		RS->BlendFactorRGBA[2] = Desc.BlendFactorRGBA[2];
		RS->BlendFactorRGBA[3] = Desc.BlendFactorRGBA[3];
		RS->SampleMask = Desc.SampleMask;
		//???store alpha ref as shader var? store clip plane enable as flag that signs to set clip plane vars?

		RenderStates.Add(RS);

		return RS.GetUnsafe();
	}

ProcessFailure:

	if (pRState) pRState->Release();
	if (pDSState) pDSState->Release();
	if (pBState) pBState->Release();

	return NULL;
}
//---------------------------------------------------------------------

PShader CD3D11GPUDriver::CreateShader(EShaderType ShaderType, const void* pData, UPTR Size)
{
	if (!pData || !Size) return NULL;

	ID3D11DeviceChild* pShader = NULL;

	switch (ShaderType)
	{
		case ShaderType_Vertex:
		{
			ID3D11VertexShader* pVS = NULL;
			pD3DDevice->CreateVertexShader(pData, Size, NULL, &pVS);
			pShader = pVS;
			break;
		}
		case ShaderType_Pixel:
		{
			ID3D11PixelShader* pPS = NULL;
			pD3DDevice->CreatePixelShader(pData, Size, NULL, &pPS);
			pShader = pPS;
			break;
		}
		case ShaderType_Geometry:
		{
			//???need stream output? or separate method? or separate shader type?
			ID3D11GeometryShader* pGS = NULL;
			pD3DDevice->CreateGeometryShader(pData, Size, NULL, &pGS);
			pShader = pGS;
			break;
		}
		case ShaderType_Hull:
		{
			ID3D11HullShader* pHS = NULL;
			pD3DDevice->CreateHullShader(pData, Size, NULL, &pHS);
			pShader = pHS;
			break;
		}
		case ShaderType_Domain:
		{
			ID3D11DomainShader* pDS = NULL;
			pD3DDevice->CreateDomainShader(pData, Size, NULL, &pDS);
			pShader = pDS;
			break;
		}
		default: return NULL;
	};

	if (!pShader) return NULL;

	PD3D11Shader Shader = n_new(Render::CD3D11Shader);
	if (!Shader->Create(pShader))
	{
		pShader->Release();
		return NULL;
	}

	return Shader.GetUnsafe();
}
//---------------------------------------------------------------------

// Pointer will be 16-byte aligned
bool CD3D11GPUDriver::MapResource(void** ppOutData, const CVertexBuffer& Resource, EResourceMapMode Mode)
{
	n_assert_dbg(Resource.IsA<CD3D11VertexBuffer>());
	if (!ppOutData) FAIL;

	ID3D11Buffer* pVB = ((const CD3D11VertexBuffer&)Resource).GetD3DBuffer();
	if (!pVB) FAIL;

	//???set locked flag on resource or can check it by API?
	//???assert or check CPU access?! or Resource.CanMap()?

	D3D11_MAP MapType;
	UPTR MapFlags;
	GetD3DMapTypeAndFlags(Mode, MapType, MapFlags);

	D3D11_MAPPED_SUBRESOURCE D3DData;
	if (FAILED(pD3DImmContext->Map(pVB, 0, MapType, MapFlags, &D3DData))) FAIL;
	*ppOutData = D3DData.pData;

	OK;
}
//---------------------------------------------------------------------

//!!!DUPLICATE CODE, move to internal MapD3DBuffer()?!
// Pointer will be 16-byte aligned
bool CD3D11GPUDriver::MapResource(void** ppOutData, const CIndexBuffer& Resource, EResourceMapMode Mode)
{
	n_assert_dbg(Resource.IsA<CD3D11IndexBuffer>());
	if (!ppOutData) FAIL;

	ID3D11Buffer* pIB = ((const CD3D11IndexBuffer&)Resource).GetD3DBuffer();
	if (!pIB) FAIL;

	//???set locked flag on resource or can check it by API?
	//???assert or check CPU access?! or Resource.CanMap()?

	D3D11_MAP MapType;
	UPTR MapFlags;
	GetD3DMapTypeAndFlags(Mode, MapType, MapFlags);

	D3D11_MAPPED_SUBRESOURCE D3DData;
	if (FAILED(pD3DImmContext->Map(pIB, 0, MapType, MapFlags, &D3DData))) FAIL;
	*ppOutData = D3DData.pData;

	OK;
}
//---------------------------------------------------------------------

// Pointer will be 16-byte aligned
bool CD3D11GPUDriver::MapResource(CImageData& OutData, const CTexture& Resource, EResourceMapMode Mode, UPTR ArraySlice, UPTR MipLevel)
{
	n_assert_dbg(Resource.IsA<CD3D11Texture>());
	ID3D11Resource* pD3DTexRsrc = ((const CD3D11Texture&)Resource).GetD3DResource();
	if (!pD3DTexRsrc) FAIL;

	//???set locked flag on SUBresource or can check it by API?
	//???assert or check CPU access?! or Resource.CanMap()?

	const CTextureDesc& TexDesc = Resource.GetDesc();

/*
	// Perform cubemap face mapping, if necesary. Can avoid it if cubemap faces are loaded in ECubeMapFace order.
	if (TexDesc.Type == Texture_Cube)
	{
		UPTR ArrayIndex = ArraySlice / 6;
		UPTR Face = ArraySlice - (ArrayIndex * 6);
		switch ((ECubeMapFace)Face)
		{
			case CubeFace_PosX:	return D3DCUBEMAP_FACE_POSITIVE_X;
			case CubeFace_NegX:	return D3DCUBEMAP_FACE_NEGATIVE_X;
			case CubeFace_PosY:	return D3DCUBEMAP_FACE_POSITIVE_Y;
			case CubeFace_NegY:	return D3DCUBEMAP_FACE_NEGATIVE_Y;
			case CubeFace_PosZ:	return D3DCUBEMAP_FACE_POSITIVE_Z;
			case CubeFace_NegZ:	return D3DCUBEMAP_FACE_NEGATIVE_Z;
		}
	}
*/

	D3D11_MAP MapType;
	UPTR MapFlags;
	GetD3DMapTypeAndFlags(Mode, MapType, MapFlags);

	D3D11_MAPPED_SUBRESOURCE D3DData;
	if (FAILED(pD3DImmContext->Map(pD3DTexRsrc, D3D11CalcSubresource(MipLevel, ArraySlice, TexDesc.MipLevels), MapType, MapFlags, &D3DData))) FAIL;

	OutData.pData = (char*)D3DData.pData;
	OutData.RowPitch = D3DData.RowPitch;
	OutData.SlicePitch = D3DData.DepthPitch;

	FAIL;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::UnmapResource(const CVertexBuffer& Resource)
{
	n_assert_dbg(Resource.IsA<CD3D11VertexBuffer>());
	//???!!!return are outstanding locks or resource was unlocked?!
	ID3D11Buffer* pVB = ((const CD3D11VertexBuffer&)Resource).GetD3DBuffer();
	if (!pVB) FAIL;
	pD3DImmContext->Unmap(pVB, 0);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::UnmapResource(const CIndexBuffer& Resource)
{
	n_assert_dbg(Resource.IsA<CD3D11IndexBuffer>());
	//???!!!return are outstanding locks or resource was unlocked?!
	ID3D11Buffer* pIB = ((const CD3D11IndexBuffer&)Resource).GetD3DBuffer();
	if (!pIB) FAIL;
	pD3DImmContext->Unmap(pIB, 0);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::UnmapResource(const CTexture& Resource, UPTR ArraySlice, UPTR MipLevel)
{
	n_assert_dbg(Resource.IsA<CD3D11Texture>());
	ID3D11Resource* pD3DTexRsrc = ((const CD3D11Texture&)Resource).GetD3DResource();
	if (!pD3DTexRsrc) FAIL;

	const CTextureDesc& TexDesc = Resource.GetDesc();

/*
	// Perform cubemap face mapping, if necesary. Can avoid it if cubemap faces are loaded in ECubeMapFace order.
	if (TexDesc.Type == Texture_Cube)
	{
		UPTR ArrayIndex = ArraySlice / 6;
		UPTR Face = ArraySlice - (ArrayIndex * 6);
		switch ((ECubeMapFace)Face)
		{
			case CubeFace_PosX:	return D3DCUBEMAP_FACE_POSITIVE_X;
			case CubeFace_NegX:	return D3DCUBEMAP_FACE_NEGATIVE_X;
			case CubeFace_PosY:	return D3DCUBEMAP_FACE_POSITIVE_Y;
			case CubeFace_NegY:	return D3DCUBEMAP_FACE_NEGATIVE_Y;
			case CubeFace_PosZ:	return D3DCUBEMAP_FACE_POSITIVE_Z;
			case CubeFace_NegZ:	return D3DCUBEMAP_FACE_NEGATIVE_Z;
		}
	}
*/

	pD3DImmContext->Unmap(pD3DTexRsrc, D3D11CalcSubresource(MipLevel, ArraySlice, TexDesc.MipLevels));
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::ReadFromD3DBuffer(void* pDest, ID3D11Buffer* pBuf, D3D11_USAGE Usage, UPTR BufferSize, UPTR Size, UPTR Offset)
{
	if (!pDest || !pBuf || !BufferSize) FAIL;

	UPTR RequestedSize = Size ? Size : BufferSize;
	UPTR SizeToCopy = n_min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	const bool IsNonMappable = (Usage == D3D11_USAGE_DEFAULT || Usage == D3D11_USAGE_IMMUTABLE);

	ID3D11Buffer* pBufToMap = NULL;
	if (IsNonMappable)
	{
		// Instead of creation may use ring buffer of precreated resources!
		D3D11_BUFFER_DESC D3DDesc;
		pBuf->GetDesc(&D3DDesc);
		D3DDesc.Usage = D3D11_USAGE_STAGING;
		D3DDesc.BindFlags = 0;
		D3DDesc.MiscFlags = 0;
		D3DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		if (FAILED(pD3DDevice->CreateBuffer(&D3DDesc, NULL, &pBufToMap))) FAIL;

		// PERF: Async, immediate reading may cause stall. Allow processing multiple read requests per call or make ReadFromResource async?
		pD3DImmContext->CopyResource(pBufToMap, pBuf);
	}
	else pBufToMap = pBuf;

	D3D11_MAPPED_SUBRESOURCE D3DData;
	if (FAILED(pD3DImmContext->Map(pBufToMap, 0, D3D11_MAP_READ, 0, &D3DData)))
	{
		if (IsNonMappable) pBufToMap->Release(); // Or return it to the ring buffer
		FAIL;
	}

	memcpy(pDest, (char*)D3DData.pData + Offset, SizeToCopy);

	pD3DImmContext->Unmap(pBufToMap, 0);
	if (IsNonMappable) pBufToMap->Release(); // Or return it to the ring buffer

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::ReadFromResource(void* pDest, const CVertexBuffer& Resource, UPTR Size, UPTR Offset)
{
	n_assert_dbg(Resource.IsA<CD3D11VertexBuffer>());
	const CD3D11VertexBuffer& VB11 = (const CD3D11VertexBuffer&)Resource;
	return ReadFromD3DBuffer(pDest, VB11.GetD3DBuffer(), VB11.GetD3DUsage(), VB11.GetSizeInBytes(), Size, Offset);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::ReadFromResource(void* pDest, const CIndexBuffer& Resource, UPTR Size, UPTR Offset)
{
	n_assert_dbg(Resource.IsA<CD3D11IndexBuffer>());
	const CD3D11IndexBuffer& IB11 = (const CD3D11IndexBuffer&)Resource;
	return ReadFromD3DBuffer(pDest, IB11.GetD3DBuffer(), IB11.GetD3DUsage(), IB11.GetSizeInBytes(), Size, Offset);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::ReadFromResource(const CImageData& Dest, const CTexture& Resource, UPTR ArraySlice, UPTR MipLevel, const Data::CBox* pRegion)
{
	n_assert_dbg(Resource.IsA<CD3D11Texture>());

	const CTextureDesc& Desc = Resource.GetDesc();
	if (!Dest.pData || MipLevel >= Desc.MipLevels) FAIL;

	UPTR RealArraySize = (Desc.Type == Texture_Cube) ? 6 * Desc.ArraySize : Desc.ArraySize;
	if (ArraySlice >= RealArraySize) FAIL;

	const CD3D11Texture& Tex11 = (const CD3D11Texture&)Resource;
	ID3D11Resource* pTexRsrc = Tex11.GetD3DResource();
	D3D11_USAGE Usage = Tex11.GetD3DUsage();
	UPTR Dims = Resource.GetDimensionCount();
	if (!pTexRsrc || !Dims) FAIL;

	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);
	UPTR BPP = CD3D11DriverFactory::DXGIFormatBitsPerPixel(DXGIFormat);
	if (!BPP) FAIL;

	UPTR TotalSizeX = n_max(Desc.Width >> MipLevel, 1);
	UPTR TotalSizeY = n_max(Desc.Height >> MipLevel, 1);
	UPTR TotalSizeZ = n_max(Desc.Depth >> MipLevel, 1);

	CCopyImageParams Params;
	Params.BitsPerPixel = BPP;

	if (!CalcValidImageRegion(pRegion, Dims, TotalSizeX, TotalSizeY, TotalSizeZ,
							  Params.Offset[0], Params.Offset[1], Params.Offset[2],
							  Params.CopySize[0], Params.CopySize[1], Params.CopySize[2]))
	{
		OK;
	}

	const bool IsNonMappable = (Usage == D3D11_USAGE_DEFAULT || Usage == D3D11_USAGE_IMMUTABLE);

	UPTR ImageCopyFlags = CopyImage_AdjustSrc;

	ID3D11Resource* pRsrcToMap = NULL;
	if (IsNonMappable)
	{
		// Instead of creation may use ring buffer of precreated resources!
		const ETextureType TexType = Desc.Type;
		switch (TexType)
		{
			case Texture_1D:
			{
				D3D11_TEXTURE1D_DESC D3DDesc;
				Tex11.GetD3DTexture1D()->GetDesc(&D3DDesc);
				D3DDesc.MipLevels = 1;
				D3DDesc.ArraySize = 1;
				D3DDesc.Width = TotalSizeX;
				D3DDesc.Usage = D3D11_USAGE_STAGING;
				D3DDesc.BindFlags = 0;
				D3DDesc.MiscFlags = 0;
				D3DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

				ID3D11Texture1D* pTex = NULL;
				if (FAILED(pD3DDevice->CreateTexture1D(&D3DDesc, NULL, &pTex))) FAIL;
				pRsrcToMap = pTex;

				break;
			}

			case Texture_2D:
			case Texture_Cube:
			{
				D3D11_TEXTURE2D_DESC D3DDesc;
				Tex11.GetD3DTexture2D()->GetDesc(&D3DDesc);
				D3DDesc.MipLevels = 1;
				D3DDesc.ArraySize = 1;
				D3DDesc.Width = TotalSizeX;
				D3DDesc.Height = TotalSizeY;
				D3DDesc.Usage = D3D11_USAGE_STAGING;
				D3DDesc.BindFlags = 0;
				D3DDesc.MiscFlags = 0;
				D3DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

				ID3D11Texture2D* pTex = NULL;
				if (FAILED(pD3DDevice->CreateTexture2D(&D3DDesc, NULL, &pTex))) FAIL;
				pRsrcToMap = pTex;

				break;
			}

			case Texture_3D:
			{
				D3D11_TEXTURE3D_DESC D3DDesc;
				Tex11.GetD3DTexture3D()->GetDesc(&D3DDesc);
				D3DDesc.MipLevels = 1;
				D3DDesc.Width = TotalSizeX;
				D3DDesc.Height = TotalSizeY;
				D3DDesc.Depth = TotalSizeZ;
				D3DDesc.Usage = D3D11_USAGE_STAGING;
				D3DDesc.BindFlags = 0;
				D3DDesc.MiscFlags = 0;
				D3DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

				ID3D11Texture3D* pTex = NULL;
				if (FAILED(pD3DDevice->CreateTexture3D(&D3DDesc, NULL, &pTex))) FAIL;
				pRsrcToMap = pTex;

				ImageCopyFlags |= CopyImage_3DImage;

				break;
			}
		};

		// PERF: Async, immediate reading may cause stall. Allow processing multiple read requests per call or make ReadFromResource async?
		pD3DImmContext->CopySubresourceRegion(pRsrcToMap, 0, 0, 0, 0, pTexRsrc, D3D11CalcSubresource(MipLevel, ArraySlice, Desc.MipLevels), NULL);
	}
	else pRsrcToMap = pTexRsrc;

	D3D11_MAPPED_SUBRESOURCE MappedTex;
	if (FAILED(pD3DImmContext->Map(pRsrcToMap, 0, D3D11_MAP_READ, 0, &MappedTex)))
	{
		if (IsNonMappable) pRsrcToMap->Release(); // Or return it to the ring buffer
		FAIL;
	}

	CImageData SrcData;
	SrcData.pData = (char*)MappedTex.pData;
	SrcData.RowPitch = MappedTex.RowPitch;
	SrcData.SlicePitch = MappedTex.DepthPitch;

	Params.TotalSize[0] = TotalSizeX;
	Params.TotalSize[1] = TotalSizeY;

	if (CD3D11DriverFactory::DXGIFormatBlockSize(DXGIFormat) > 1)
		ImageCopyFlags |= CopyImage_BlockCompressed;

	CopyImage(SrcData, Dest, ImageCopyFlags, Params);

	pD3DImmContext->Unmap(pRsrcToMap, 0);
	if (IsNonMappable) pRsrcToMap->Release(); // Or return it to the ring buffer

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::WriteToD3DBuffer(ID3D11Buffer* pBuf, D3D11_USAGE Usage, UPTR BufferSize, const void* pData, UPTR Size, UPTR Offset)
{
	if (!pBuf || Usage == D3D11_USAGE_IMMUTABLE || !pData || !BufferSize) FAIL;

	UPTR RequestedSize = Size ? Size : BufferSize;
	UPTR SizeToCopy = n_min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	const int UpdateWhole = (!Offset && SizeToCopy == BufferSize);

	if (Usage == D3D11_USAGE_DEFAULT) //???update staging here too? need perf test!
	{
		if (UpdateWhole) pD3DImmContext->UpdateSubresource(pBuf, 0, NULL, pData, 0, 0);
		else
		{
			D3D11_BOX D3DBox;
			D3DBox.left = Offset;
			D3DBox.right = Offset + SizeToCopy;
			D3DBox.top = 0;
			D3DBox.bottom = 1;
			D3DBox.front = 0;
			D3DBox.back = 1;
			pD3DImmContext->UpdateSubresource(pBuf, 0, &D3DBox, pData, 0, 0);
		}
	}
	else
	{
		D3D11_MAP MapType = (Usage == D3D11_USAGE_DYNAMIC && UpdateWhole) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
		D3D11_MAPPED_SUBRESOURCE D3DData;
		if (FAILED(pD3DImmContext->Map(pBuf, 0, MapType, 0, &D3DData))) FAIL;
		memcpy(((char*)D3DData.pData) + Offset, pData, SizeToCopy);
		pD3DImmContext->Unmap(pBuf, 0);
	}

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::WriteToResource(CVertexBuffer& Resource, const void* pData, UPTR Size, UPTR Offset)
{
	n_assert_dbg(Resource.IsA<CD3D11VertexBuffer>());
	const CD3D11VertexBuffer& VB11 = (const CD3D11VertexBuffer&)Resource;
	return WriteToD3DBuffer(VB11.GetD3DBuffer(), VB11.GetD3DUsage(), VB11.GetSizeInBytes(), pData, Size, Offset);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::WriteToResource(CIndexBuffer& Resource, const void* pData, UPTR Size, UPTR Offset)
{
	n_assert_dbg(Resource.IsA<CD3D11IndexBuffer>());
	const CD3D11IndexBuffer& IB11 = (const CD3D11IndexBuffer&)Resource;
	return WriteToD3DBuffer(IB11.GetD3DBuffer(), IB11.GetD3DUsage(), IB11.GetSizeInBytes(), pData, Size, Offset);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::WriteToResource(CTexture& Resource, const CImageData& SrcData, UPTR ArraySlice, UPTR MipLevel, const Data::CBox* pRegion)
{
	n_assert_dbg(Resource.IsA<CD3D11Texture>());

	const CTextureDesc& Desc = Resource.GetDesc();
	if (!SrcData.pData || MipLevel >= Desc.MipLevels) FAIL;

	UPTR RealArraySize = (Desc.Type == Texture_Cube) ? 6 * Desc.ArraySize : Desc.ArraySize;
	if (ArraySlice >= RealArraySize) FAIL;

	const CD3D11Texture& Tex11 = (const CD3D11Texture&)Resource;
	ID3D11Resource* pTexRsrc = Tex11.GetD3DResource();
	D3D11_USAGE Usage = Tex11.GetD3DUsage();
	UPTR Dims = Resource.GetDimensionCount();
	if (!pTexRsrc || Usage == D3D11_USAGE_IMMUTABLE || !Dims) FAIL;

	UPTR TotalSizeX = n_max(Desc.Width >> MipLevel, 1);
	UPTR TotalSizeY = n_max(Desc.Height >> MipLevel, 1);
	UPTR TotalSizeZ = n_max(Desc.Depth >> MipLevel, 1);

	CCopyImageParams Params;

	UPTR OffsetX, OffsetY, OffsetZ, SizeX, SizeY, SizeZ;
	if (!CalcValidImageRegion(pRegion, Dims, TotalSizeX, TotalSizeY, TotalSizeZ,
							  OffsetX, OffsetY, OffsetZ, SizeX, SizeY, SizeZ))
	{
		OK;
	}

	if (Usage == D3D11_USAGE_DEFAULT) //???update staging here too? need perf test!
	{
		//!!!only non-DS and non-MSAA!
		//!!!can also use staging ring buffer! PERF?

		D3D11_BOX D3DBox; // In texels
		D3DBox.left = OffsetX;
		D3DBox.right = OffsetX + SizeX;
		D3DBox.top = OffsetY;
		D3DBox.bottom = OffsetY + SizeY;
		D3DBox.front = OffsetZ;
		D3DBox.back = OffsetZ + SizeZ;
		pD3DImmContext->UpdateSubresource(pTexRsrc, D3D11CalcSubresource(MipLevel, ArraySlice, Desc.MipLevels),
										  &D3DBox, SrcData.pData, SrcData.RowPitch, SrcData.SlicePitch);

		OK;
	}

	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);

	// No format conversion for now, src & dest texel formats must match
	UPTR BPP = CD3D11DriverFactory::DXGIFormatBitsPerPixel(DXGIFormat);
	if (!BPP) FAIL;

	D3D11_MAP MapType;
	if (Usage == D3D11_USAGE_DYNAMIC)
	{
		const bool UpdateWhole = !pRegion || (SizeX == TotalSizeX && (Dims < 2 || SizeY == TotalSizeY && (Dims < 3 || SizeZ == TotalSizeZ)));
		MapType = UpdateWhole ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
	}
	else MapType = D3D11_MAP_WRITE;

	D3D11_MAPPED_SUBRESOURCE D3DData;
	if (FAILED(pD3DImmContext->Map(pTexRsrc, 0, MapType, 0, &D3DData))) FAIL;

	CImageData DestData;
	DestData.pData = (char*)D3DData.pData;
	DestData.RowPitch = D3DData.RowPitch;
	DestData.SlicePitch = D3DData.DepthPitch;

	Params.BitsPerPixel = BPP;
	Params.Offset[0] = OffsetX;
	Params.Offset[1] = OffsetY;
	Params.Offset[2] = OffsetZ;
	Params.CopySize[0] = SizeX;
	Params.CopySize[1] = SizeY;
	Params.CopySize[2] = SizeZ;
	Params.TotalSize[0] = TotalSizeX;
	Params.TotalSize[1] = TotalSizeY;

	UPTR ImageCopyFlags = CopyImage_AdjustDest;
	if (CD3D11DriverFactory::DXGIFormatBlockSize(DXGIFormat) > 1)
		ImageCopyFlags |= CopyImage_BlockCompressed;
	if (Desc.Type == Texture_3D) ImageCopyFlags |= CopyImage_3DImage;

	CopyImage(SrcData, DestData, ImageCopyFlags, Params);

	pD3DImmContext->Unmap(pTexRsrc, 0);

	OK;
}
//---------------------------------------------------------------------

// Especially useful for VRAM-only D3D11_USAGE_DEFAULT buffers. May not support partial updates.
bool CD3D11GPUDriver::WriteToResource(CConstantBuffer& Resource, const void* pData, UPTR Size, UPTR Offset)
{
	n_assert_dbg(Resource.IsA<CD3D11ConstantBuffer>());
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)Resource;

	// Can't write to D3D 11.0 constant buffers partially
	if (Offset || (Size && Size != CB11.GetSizeInBytes())) FAIL;

	ID3D11Buffer* pBuffer = CB11.GetD3DBuffer();
	D3D11_USAGE D3DUsage = CB11.GetD3DUsage();

	if (D3DUsage == D3D11_USAGE_DEFAULT)
	{
		pD3DImmContext->UpdateSubresource(pBuffer, 0, NULL, pData, 0, 0);
	}
	else if (D3DUsage == D3D11_USAGE_DYNAMIC)
	{
		if (CB11.GetMappedVRAM()) FAIL; // Buffer is already mapped, can't write with a commit
		D3D11_MAPPED_SUBRESOURCE MappedSubRsrc;
		if (FAILED(pD3DImmContext->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubRsrc))) FAIL;
		memcpy(MappedSubRsrc.pData, pData, CB11.GetSizeInBytes());
		pD3DImmContext->Unmap(pBuffer, 0);
	}
	else FAIL;

	CB11.ResetRAMCopy(pData);

	OK;
}
//---------------------------------------------------------------------

// Only dynamic buffers and buffers with a RAM copy support setting constants.
// Default buffers support only the whole buffer update via WriteToResource().
bool CD3D11GPUDriver::BeginShaderConstants(CConstantBuffer& Buffer)
{
	n_assert_dbg(Buffer.IsA<CD3D11ConstantBuffer>());
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)Buffer;

	ID3D11Buffer* pBuffer = CB11.GetD3DBuffer();
	D3D11_USAGE D3DUsage = CB11.GetD3DUsage();

	// Invalid or read-only
	if (!pBuffer || D3DUsage == D3D11_USAGE_IMMUTABLE) FAIL;

	// Writes to the RAM copy, no additional actions required
	if (CB11.UsesRAMCopy())
	{
		CB11.OnBegin();
		OK;
	}

	// VRAM-only, non-mappable, can't write constants without a RAM copy
	if (D3DUsage != D3D11_USAGE_DYNAMIC) FAIL;

	if (!CB11.GetMappedVRAM())
	{
		D3D11_MAPPED_SUBRESOURCE MappedSubRsrc;
		if (FAILED(pD3DImmContext->Map(pBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubRsrc))) FAIL;
		CB11.OnBegin(MappedSubRsrc.pData);
	}

	OK;
}
//---------------------------------------------------------------------

//!!!may use abstract CShaderConst, CShaderConstFloatArray, CD3D11ShaderConstFloatArray: public CD3D11ShaderArrayConst<float> etc and cache offset or any
//other precomputed location there for fast setting!
bool CD3D11GPUDriver::SetShaderConstant(CConstantBuffer& Buffer, HConst hConst, UPTR ElementIndex, const void* pData, UPTR Size)
{
	if (!hConst || !pData || !Size) FAIL;

	n_assert_dbg(Buffer.IsA<CD3D11ConstantBuffer>());
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)Buffer;

	CUSMConstMeta* pMeta = (CUSMConstMeta*)IShaderMetadata::GetHandleData(hConst);
	if (!pMeta) FAIL;

	UPTR Offset = pMeta->Offset; 

	if (ElementIndex)
	{
		CUSMBufferMeta* pBufMeta = (CUSMBufferMeta*)IShaderMetadata::GetHandleData(pMeta->BufferHandle);
		if (!pBufMeta) FAIL;
		if (pBufMeta->Type == USMBuffer_Structured)
			Offset += pBufMeta->Size * ElementIndex;
		else if (pBufMeta->Type == USMBuffer_Texture)
			Offset += pMeta->ElementSize * pMeta->ElementCount * ElementIndex;
	}

	//!!!structured buffer must validate Offset against its size inside!
	CB11.WriteData(Offset, pData, Size);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::CommitShaderConstants(CConstantBuffer& Buffer)
{
	n_assert_dbg(Buffer.IsA<CD3D11ConstantBuffer>());
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)Buffer;

	ID3D11Buffer* pBuffer = CB11.GetD3DBuffer();
	D3D11_USAGE D3DUsage = CB11.GetD3DUsage();

	if (CB11.UsesRAMCopy())
	{
		// Commit RAM copy to VRAM or to a staging resource

		if (!CB11.IsDirty()) OK;

		if (D3DUsage == D3D11_USAGE_DEFAULT)
		{
			pD3DImmContext->UpdateSubresource(pBuffer, 0, NULL, CB11.GetRAMCopy(), 0, 0);
		}
		else if (D3DUsage == D3D11_USAGE_DYNAMIC || D3DUsage == D3D11_USAGE_STAGING)
		{
			D3D11_MAPPED_SUBRESOURCE MappedSubRsrc;
			if (FAILED(pD3DImmContext->Map(pBuffer, 0, (D3DUsage == D3D11_USAGE_DYNAMIC) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE, 0, &MappedSubRsrc))) FAIL;
			memcpy(MappedSubRsrc.pData, CB11.GetRAMCopy(), CB11.GetSizeInBytes());
			pD3DImmContext->Unmap(pBuffer, 0);
		}
		else FAIL;
	}
	else if (D3DUsage == D3D11_USAGE_DYNAMIC)
	{
		// Unmap previously mapped VRAM-only dynamic buffer

		if (CB11.GetMappedVRAM())
		{
			n_assert_dbg(CB11.IsDirty()); // Ensure something was written. Else all the buffer contents are discarded.
			pD3DImmContext->Unmap(pBuffer, 0);
		}
	}

	CB11.OnCommit();

	OK;
}
//---------------------------------------------------------------------

D3D_DRIVER_TYPE CD3D11GPUDriver::GetD3DDriverType(EGPUDriverType DriverType)
{
	// WARP adapter is skipped.
	// You also create the render-only device when you specify D3D_DRIVER_TYPE_WARP in the DriverType parameter
	// of D3D11CreateDevice because the WARP device also uses the render-only WARP adapter (c) Docs
	switch (DriverType)
	{
		case GPU_AutoSelect:	return D3D_DRIVER_TYPE_UNKNOWN;
		case GPU_Hardware:		return D3D_DRIVER_TYPE_HARDWARE;
		case GPU_Reference:		return D3D_DRIVER_TYPE_REFERENCE;
		case GPU_Software:		return D3D_DRIVER_TYPE_SOFTWARE;
		case GPU_Null:			return D3D_DRIVER_TYPE_NULL;
		default:				Sys::Error("CD3D11GPUDriver::GetD3DDriverType() > invalid GPU driver type"); return D3D_DRIVER_TYPE_UNKNOWN;
	};
}
//---------------------------------------------------------------------

EGPUDriverType CD3D11GPUDriver::GetDEMDriverType(D3D_DRIVER_TYPE DriverType)
{
	switch (DriverType)
	{
		case D3D_DRIVER_TYPE_UNKNOWN:	return GPU_AutoSelect;
		case D3D_DRIVER_TYPE_HARDWARE:	return GPU_Hardware;
		case D3D_DRIVER_TYPE_WARP:		return GPU_Reference;
		case D3D_DRIVER_TYPE_REFERENCE:	return GPU_Reference;
		case D3D_DRIVER_TYPE_SOFTWARE:	return GPU_Software;
		case D3D_DRIVER_TYPE_NULL:		return GPU_Null;
		default:						Sys::Error("CD3D11GPUDriver::GetD3DDriverType() > invalid D3D_DRIVER_TYPE"); return GPU_AutoSelect;
	};
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::GetUsageAccess(UPTR InAccessFlags, bool InitDataProvided, D3D11_USAGE& OutUsage, UINT& OutCPUAccess)
{
	if (InAccessFlags == Access_GPU_Read || InAccessFlags == 0)
	{
		OutUsage = InitDataProvided ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
	}
	else if (InAccessFlags == (Access_GPU_Read | Access_GPU_Write) ||
			 InAccessFlags == Access_GPU_Write)
	{
		OutUsage = D3D11_USAGE_DEFAULT;
	}
	else if (InAccessFlags == (Access_GPU_Read | Access_CPU_Write)) //???allow GPU_Write?
	{
		OutUsage = D3D11_USAGE_DYNAMIC;
	}
	else //???are there any unsupported combinations? user wants to get a resource with a specified access or NULL!
	{
		OutUsage = D3D11_USAGE_STAGING; // Can't be a depth-stencil buffer or a multisampled render target
	}

	OutCPUAccess = 0;
	if (InAccessFlags & Access_CPU_Read) OutCPUAccess |= D3D11_CPU_ACCESS_READ;
	if (InAccessFlags & Access_CPU_Write) OutCPUAccess |= D3D11_CPU_ACCESS_WRITE;
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::GetD3DMapTypeAndFlags(EResourceMapMode MapMode, D3D11_MAP& OutMapType, UPTR& OutMapFlags)
{
	OutMapFlags = 0;
	switch (MapMode)
	{
		case Map_Read:
		{
			OutMapType = D3D11_MAP_READ;
			return;
		}
		case Map_Write:
		{
			OutMapType = D3D11_MAP_WRITE;
			return;
		}
		case Map_ReadWrite:
		{
			OutMapType = D3D11_MAP_READ_WRITE;
			return;
		}
		case Map_WriteDiscard:
		{
			OutMapType = D3D11_MAP_WRITE_DISCARD;
			return;
		}
		case Map_WriteNoOverwrite:
		{
			OutMapType = D3D11_MAP_WRITE_NO_OVERWRITE;
			return;
		}
		default: Sys::Error("CD3D11GPUDriver::GetD3DMapTypeAndFlags() > Invalid map mode\n"); return;
	}
}
//---------------------------------------------------------------------

D3D11_COMPARISON_FUNC CD3D11GPUDriver::GetD3DCmpFunc(ECmpFunc Func)
{
	switch (Func)
	{
		case Cmp_Never:			return D3D11_COMPARISON_NEVER;
		case Cmp_Less:			return D3D11_COMPARISON_LESS;
		case Cmp_LessEqual:		return D3D11_COMPARISON_LESS_EQUAL;
		case Cmp_Greater:		return D3D11_COMPARISON_GREATER;
		case Cmp_GreaterEqual:	return D3D11_COMPARISON_GREATER_EQUAL;
		case Cmp_Equal:			return D3D11_COMPARISON_EQUAL;
		case Cmp_NotEqual:		return D3D11_COMPARISON_NOT_EQUAL;
		case Cmp_Always:		return D3D11_COMPARISON_ALWAYS;
		default: Sys::Error("CD3D11GPUDriver::GetD3DCmpFunc() > invalid function"); return D3D11_COMPARISON_NEVER;
	}
}
//---------------------------------------------------------------------

D3D11_STENCIL_OP CD3D11GPUDriver::GetD3DStencilOp(EStencilOp Operation)
{
	switch (Operation)
	{
		case StencilOp_Keep:	return D3D11_STENCIL_OP_KEEP;
		case StencilOp_Zero:	return D3D11_STENCIL_OP_ZERO;
		case StencilOp_Replace:	return D3D11_STENCIL_OP_REPLACE;
		case StencilOp_Inc:		return D3D11_STENCIL_OP_INCR;
		case StencilOp_IncSat:	return D3D11_STENCIL_OP_INCR_SAT;
		case StencilOp_Dec:		return D3D11_STENCIL_OP_DECR;
		case StencilOp_DecSat:	return D3D11_STENCIL_OP_DECR_SAT;
		case StencilOp_Invert:	return D3D11_STENCIL_OP_INVERT;
		default: Sys::Error("CD3D11GPUDriver::GetD3DStencilOp() > invalid operation"); return D3D11_STENCIL_OP_KEEP;
	}
}
//---------------------------------------------------------------------

D3D11_BLEND CD3D11GPUDriver::GetD3DBlendArg(EBlendArg Arg)
{
	switch (Arg)
	{
		case BlendArg_Zero:				return D3D11_BLEND_ZERO;
		case BlendArg_One:				return D3D11_BLEND_ONE;
		case BlendArg_SrcColor:			return D3D11_BLEND_SRC_COLOR;
		case BlendArg_InvSrcColor:		return D3D11_BLEND_INV_SRC_COLOR;
		case BlendArg_Src1Color:		return D3D11_BLEND_SRC1_COLOR;
		case BlendArg_InvSrc1Color:		return D3D11_BLEND_INV_SRC1_COLOR;
		case BlendArg_SrcAlpha:			return D3D11_BLEND_SRC_ALPHA;
		case BlendArg_SrcAlphaSat:		return D3D11_BLEND_SRC_ALPHA_SAT;
		case BlendArg_InvSrcAlpha:		return D3D11_BLEND_INV_SRC_ALPHA;
		case BlendArg_Src1Alpha:		return D3D11_BLEND_SRC1_ALPHA;
		case BlendArg_InvSrc1Alpha:		return D3D11_BLEND_INV_SRC1_ALPHA;
		case BlendArg_DestColor:		return D3D11_BLEND_DEST_COLOR;
		case BlendArg_InvDestColor:		return D3D11_BLEND_INV_DEST_COLOR;
		case BlendArg_DestAlpha:		return D3D11_BLEND_DEST_ALPHA;
		case BlendArg_InvDestAlpha:		return D3D11_BLEND_INV_DEST_ALPHA;
		case BlendArg_BlendFactor:		return D3D11_BLEND_BLEND_FACTOR;
		case BlendArg_InvBlendFactor:	return D3D11_BLEND_INV_BLEND_FACTOR;
		default: Sys::Error("CD3D11GPUDriver::GetD3DStencilOp() > invalid argument"); return D3D11_BLEND_ZERO;
	}
}
//---------------------------------------------------------------------

D3D11_BLEND_OP CD3D11GPUDriver::GetD3DBlendOp(EBlendOp Operation)
{
	switch (Operation)
	{
		case BlendOp_Add:		return D3D11_BLEND_OP_ADD;
		case BlendOp_Sub:		return D3D11_BLEND_OP_SUBTRACT;
		case BlendOp_RevSub:	return D3D11_BLEND_OP_REV_SUBTRACT;
		case BlendOp_Min:		return D3D11_BLEND_OP_MIN;
		case BlendOp_Max:		return D3D11_BLEND_OP_MAX;
		default: Sys::Error("CD3D11GPUDriver::GetD3DBlendOp() > invalid operation"); return D3D11_BLEND_OP_ADD;
	}
}
//---------------------------------------------------------------------

D3D11_TEXTURE_ADDRESS_MODE CD3D11GPUDriver::GetD3DTexAddressMode(ETexAddressMode Mode)
{
	switch (Mode)
	{
		case TexAddr_Wrap:			return D3D11_TEXTURE_ADDRESS_WRAP;
		case TexAddr_Mirror:		return D3D11_TEXTURE_ADDRESS_MIRROR;
		case TexAddr_Clamp:			return D3D11_TEXTURE_ADDRESS_CLAMP;
		case TexAddr_Border:		return D3D11_TEXTURE_ADDRESS_BORDER;
		case TexAddr_MirrorOnce:	return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
		default: Sys::Error("CD3D11GPUDriver::GetD3DTexAddressMode() > invalid mode"); return D3D11_TEXTURE_ADDRESS_WRAP;
	}
}
//---------------------------------------------------------------------

D3D11_FILTER CD3D11GPUDriver::GetD3DTexFilter(ETexFilter Filter, bool Comparison)
{
	switch (Filter)
	{
		case TexFilter_MinMagMip_Point:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;
		case TexFilter_MinMag_Point_Mip_Linear:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR : D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
		case TexFilter_Min_Point_Mag_Linear_Mip_Point:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT : D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
		case TexFilter_Min_Point_MagMip_Linear:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR : D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR;
		case TexFilter_Min_Linear_MagMip_Point:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT : D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT;
		case TexFilter_Min_Linear_Mag_Point_Mip_Linear:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR : D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		case TexFilter_MinMag_Linear_Mip_Point:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT : D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
		case TexFilter_MinMagMip_Linear:
			return Comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		case TexFilter_Anisotropic:
			return Comparison ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
		default: Sys::Error("CD3D11GPUDriver::GetD3DTexFilter() > invalid mode"); return D3D11_FILTER_MIN_MAG_MIP_POINT;
	}
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D11SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			DestroySwapChain(i);
			OK; // Only one swap chain is allowed for each window
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D11SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			ResizeSwapChain(i, pWnd->GetWidth(), pWnd->GetHeight());
			OK; // Only one swap chain is allowed for each window
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D11SwapChain& SC = SwapChains[i];
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

/*
case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			return 0;
		}
*/

}
