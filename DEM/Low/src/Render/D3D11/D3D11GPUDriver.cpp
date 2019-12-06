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
#include <Render/D3D11/USMShaderMetadata.h>
#include <Render/RenderStateDesc.h>
#include <Render/SamplerDesc.h>
#include <Render/ShaderLibrary.h>
#include <Render/TextureData.h>
#include <Render/ImageUtils.h>
#include <Events/EventServer.h>
#include <System/Win32/OSWindowWin32.h>
#include <System/SystemEvents.h>
#include <IO/IOServer.h> //!!!DBG TMP!
#include <IO/BinaryReader.h>
#include <Data/RAMData.h>
#include <string>
#ifdef DEM_STATS
#include <Core/CoreServer.h>
#include <Data/StringUtils.h>
#endif
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

#undef min
#undef max

//!!!D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE!
//each scissor rect belongs to a viewport

namespace Render
{
RTTI_CLASS_IMPL(Render::CD3D11GPUDriver, Render::CGPUDriver);

CD3D11GPUDriver::CD3D11GPUDriver(CD3D11DriverFactory& DriverFactory)
	: _DriverFactory(&DriverFactory)
	, SwapChains(1, 1)
	, CurrSRV(16, 16)
	, RenderStates(32, 32)
	, Samplers(16, 16)
{
}
//---------------------------------------------------------------------

CD3D11GPUDriver::~CD3D11GPUDriver()
{
	Release();
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::Init(UPTR AdapterNumber, EGPUDriverType DriverType)
{
	if (!CGPUDriver::Init(AdapterNumber, DriverType)) FAIL;

	n_assert(AdapterID == Adapter_AutoSelect || _DriverFactory->AdapterExists(AdapterID));

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

	// If we use nullptr adapter (Adapter_AutoSelect), new DXGI factory will be created. We avoid it.
	IDXGIAdapter1* pAdapter = nullptr;
	if (AdapterID == Adapter_AutoSelect) AdapterID = 0;
	if (!SUCCEEDED(_DriverFactory->GetDXGIFactory()->EnumAdapters1(AdapterID, &pAdapter))) FAIL;

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

	HRESULT hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, CreateFlags,
								   FeatureLevels, FeatureLevelCount, D3D11_SDK_VERSION,
								   &pD3DDevice, &D3DFeatureLevel, &pD3DImmContext);

	//if (hr == E_INVALIDARG)
	//{
	//	// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
	//	hr = D3D11CreateDevice(	pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, CreateFlags, FeatureLevels + 1, FeatureLevelCount - 1,
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
	MaxViewportCount = std::min(MaxViewportCountCaps, VP_OR_SR_SET_FLAG_COUNT);
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

	ID3D11Texture2D* pBackBuffer = nullptr;
	if (FAILED(SC.pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer)))) FAIL;
	
	ID3D11RenderTargetView* pRTV = nullptr;
	HRESULT hr = pD3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pRTV);
	pBackBuffer->Release();
	if (FAILED(hr)) FAIL;

	if (!SC.BackBufferRT.IsValidPtr()) SC.BackBufferRT = n_new(CD3D11RenderTarget);
	if (!SC.BackBufferRT->As<CD3D11RenderTarget>()->Create(pRTV, nullptr))
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

	for (UPTR i = 0; i < TmpStructuredBuffers.GetCount(); ++i)
	{
		CTmpCB* pHead = TmpStructuredBuffers.ValueAt(i);
		while (pHead)
		{
			CTmpCB* pNextHead = pHead->pNext;
			TmpCBPool.Destroy(pHead);
			pHead = pNextHead;
		}
	}
	TmpStructuredBuffers.Clear();

	for (UPTR i = 0; i < TmpTextureBuffers.GetCount(); ++i)
	{
		CTmpCB* pHead = TmpTextureBuffers.ValueAt(i);
		while (pHead)
		{
			CTmpCB* pNextHead = pHead->pNext;
			TmpCBPool.Destroy(pHead);
			pHead = pNextHead;
		}
	}
	TmpTextureBuffers.Clear();

	for (UPTR i = 0; i < TmpConstantBuffers.GetCount(); ++i)
	{
		CTmpCB* pHead = TmpConstantBuffers.ValueAt(i);
		while (pHead)
		{
			CTmpCB* pNextHead = pHead->pNext;
			TmpCBPool.Destroy(pHead);
			pHead = pNextHead;
		}
	}
	TmpConstantBuffers.Clear();

	while (pPendingCBHead)
	{
		CTmpCB* pNextHead = pPendingCBHead->pNext;
		TmpCBPool.Destroy(pPendingCBHead);
		pPendingCBHead = pNextHead;
	}

	TmpCBPool.Clear();

	VertexLayouts.Clear();
	RenderStates.Clear();
	Samplers.Clear();

	//!!!if code won't be reused in Reset(), call DestroySwapChain()!
	for (UPTR i = 0; i < SwapChains.GetCount() ; ++i)
		if (SwapChains[i].IsValid()) SwapChains[i].Destroy();
	SwapChains.Clear();

	SAFE_DELETE_ARRAY(CurrVP);
	SAFE_DELETE_ARRAY(CurrSR);
	CurrSRV.Clear();
	CurrVB.SetSize(0);
	CurrVBOffset.SetSize(0);
	CurrCB.SetSize(0);
	CurrSS.SetSize(0);
	CurrRT.SetSize(0);

	CurrRS = nullptr;
	NewRS = nullptr;
	CurrVL = nullptr;

	//!!!ReleaseQueries();

	pD3DImmContext->ClearState();

//#if (DEM_RENDER_DEBUG != 0)
//	pD3DImmContext->Flush();
//#endif

	SAFE_RELEASE(pD3DImmContext);

//#if DEM_RENDER_DEBUG
//	ID3D11Debug* pD3D11Debug = nullptr;
//	pD3DDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&pD3D11Debug));
//	if (pD3D11Debug)
//	{
//		pD3D11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
//		pD3D11Debug->Release();
//	}
//#endif

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
		case Caps_VSTex_R16:
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

int CD3D11GPUDriver::CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, DEM::Sys::COSWindow* pWindow)
{
	if (!pWindow || !pWindow->IsA<DEM::Sys::COSWindowWin32>())
	{
		n_assert2(false, "CD3D9GPUDriver::CreateSwapChain() > invalid or unsupported window passed");
		return -1;
	}

	auto pWndWin32 = static_cast<DEM::Sys::COSWindowWin32*>(pWindow);

	//???or destroy and recreate with new params?
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.Get() == pWindow) return ERR_CREATION_ERROR;

	UPTR BBWidth = BackBufferDesc.Width, BBHeight = BackBufferDesc.Height;
	PrepareWindowAndBackBufferSize(*pWindow, BBWidth, BBHeight);

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
	SCDesc.BufferCount = std::min<UPTR>(BackBufferCount, DXGI_MAX_SWAP_CHAIN_BUFFERS);
	SCDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	if (BackBufferDesc.UseAsShaderInput)
		SCDesc.BufferUsage |= DXGI_USAGE_SHADER_INPUT;
	SCDesc.Windowed = TRUE; // Recommended, use SwitchToFullscreen()
	SCDesc.OutputWindow = pWndWin32->GetHWND();
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

	IDXGIFactory1* pDXGIFactory = _DriverFactory->GetDXGIFactory();
	IDXGISwapChain* pSwapChain = nullptr;
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

	ItSC->TargetWindow = pWindow;
	ItSC->LastWindowRect = pWindow->GetRect();
	ItSC->TargetDisplay = nullptr;
	ItSC->Desc = SwapChainDesc;

	// Disable DXGI reaction on Alt+Enter & PrintScreen
	pDXGIFactory->MakeWindowAssociation(pWndWin32->GetHWND(), DXGI_MWA_NO_WINDOW_CHANGES);

	pWindow->Subscribe<CD3D11GPUDriver>(CStrID("OnToggleFullscreen"), this, &CD3D11GPUDriver::OnOSWindowToggleFullscreen, &ItSC->Sub_OnToggleFullscreen);
	pWindow->Subscribe<CD3D11GPUDriver>(CStrID("OnClosing"), this, &CD3D11GPUDriver::OnOSWindowClosing, &ItSC->Sub_OnClosing);
	if (SwapChainDesc.Flags.Is(SwapChain_AutoAdjustSize))
		pWindow->Subscribe<CD3D11GPUDriver>(Event::OSWindowResized::RTTI, this, &CD3D11GPUDriver::OnOSWindowResized, &ItSC->Sub_OnSizeChanged);

	return SwapChains.IndexOf(ItSC);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::DestroySwapChain(UPTR SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	
	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		if (CurrRT[i].Get() == SC.BackBufferRT.Get())
		{
			SetRenderTarget(i, nullptr);
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
// Does not resize an OS window, because often is called in response to an OS window resize
//???bool ResizeOSWindow?
bool CD3D11GPUDriver::ResizeSwapChain(UPTR SwapChainID, unsigned int Width, unsigned int Height)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D11SwapChain& SC = SwapChains[SwapChainID];

//!!!DBG TMP!
BOOL FScr;
SC.pSwapChain->GetFullscreenState(&FScr, nullptr);
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
	//one as command (ResizeTarget), one as handler (ResizeBuffers), second can be OnOSWindowResized handler

	UPTR RemovedRTIdx;
	for (RemovedRTIdx = 0; RemovedRTIdx < CurrRT.GetCount(); ++RemovedRTIdx)
		if (CurrRT[RemovedRTIdx] == SC.BackBufferRT)
		{
			CurrRT[RemovedRTIdx] = nullptr;
			//!!!commit changes! pD3DImmContext->OMSetRenderTargets(0, nullptr, nullptr);
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

	if (SC.TargetWindow->IsChild()) FAIL;

	if (pDisplay) SC.TargetDisplay = pDisplay;
	else
	{
		IDXGIOutput* pOutput = nullptr;
		if (FAILED(SC.pSwapChain->GetContainingOutput(&pOutput))) FAIL;
		SC.TargetDisplay = _DriverFactory->CreateDisplayDriver(pOutput);
		if (!SC.TargetDisplay)
		{
			pOutput->Release();
			FAIL;
		}
	}

	IDXGIOutput* pDXGIOutput = SC.TargetDisplay->As<CD3D11DisplayDriver>()->GetDXGIOutput();
	n_assert(pDXGIOutput);

	// If pMode is nullptr, DXGI will default mode to a desktop mode if
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
		// We find closest supported mode instead.
		// DX 11.1: IDXGIOutput1::FindClosestMatchingMode1()
		if (FAILED(pDXGIOutput->FindClosestMatchingMode(&RequestedDXGIMode, &DXGIMode, pD3DDevice)))
		{
			SC.TargetDisplay = nullptr;
			FAIL;
		}

		// NB: it is recommended to call ResizeTarget() _before_ SetFullscreenState()
		if (FAILED(SC.pSwapChain->ResizeTarget(&DXGIMode)))
		{
			SC.TargetDisplay = nullptr;
			FAIL;
		}
	}

	SC.LastWindowRect = SC.TargetWindow->GetRect();

	HRESULT hr = SC.pSwapChain->SetFullscreenState(TRUE, pDXGIOutput);
	if (FAILED(hr))
	{
		if (hr == DXGI_STATUS_MODE_CHANGE_IN_PROGRESS) OK;
		else FAIL; //???resize back?
	}

	// After calling SetFullscreenState, it is advisable to call ResizeTarget again with the
	// RefreshRate member of DXGI_MODE_DESC zeroed out. This amounts to a no-operation instruction
	// in DXGI, but it can avoid issues with the refresh rate. (c) Docs
	if (pMode)
	{
		DXGIMode.RefreshRate.Numerator = 0;
		DXGIMode.RefreshRate.Denominator = 0;
		if (FAILED(SC.pSwapChain->ResizeTarget(&DXGIMode)))
		{
			SC.pSwapChain->SetFullscreenState(FALSE, nullptr);
			SC.TargetDisplay = nullptr;
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

	// Windowed viewport can be shared between several monitors
	SC.TargetDisplay = nullptr;

	HRESULT hr = SC.pSwapChain->SetFullscreenState(FALSE, nullptr);
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

DEM::Sys::COSWindow* CD3D11GPUDriver::GetSwapChainWindow(UPTR SwapChainID) const
{
	return SwapChainExists(SwapChainID) ? SwapChains[SwapChainID].TargetWindow.Get() : nullptr;
}
//---------------------------------------------------------------------

PDisplayDriver CD3D11GPUDriver::GetSwapChainDisplay(UPTR SwapChainID) const
{
	if (!SwapChainExists(SwapChainID)) return nullptr;

	auto& SC = SwapChains[SwapChainID];
	if (SC.TargetDisplay) return SC.TargetDisplay;

	// Windowed swap chain has no dedicated display, return the one
	// that contains the most part of the swap chain window
	IDXGIOutput* pOutput = nullptr;
	if (FAILED(SC.pSwapChain->GetContainingOutput(&pOutput))) FAIL;
	auto Display = _DriverFactory->CreateDisplayDriver(pOutput);
	if (!Display) pOutput->Release();
	return Display;
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
			CD3D11RenderTarget* pRT = CurrRT[i].Get();
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
	if (CurrVL.Get() == pVLayout) OK;
	CurrVL = static_cast<CD3D11VertexLayout*>(pVLayout);
	CurrDirtyFlags.Set(GPU_Dirty_VL);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetVertexBuffer(UPTR Index, CVertexBuffer* pVB, UPTR OffsetVertex)
{
	if (Index >= CurrVB.GetCount()) FAIL;
	UPTR Offset = pVB ? OffsetVertex * pVB->GetVertexLayout()->GetVertexSizeInBytes() : 0;
	if (CurrVB[Index].Get() == pVB && CurrVBOffset[Index] == Offset) OK;
	CurrVB[Index] = (CD3D11VertexBuffer*)pVB;
	CurrVBOffset[Index] = Offset;
	CurrDirtyFlags.Set(GPU_Dirty_VB);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetIndexBuffer(CIndexBuffer* pIB)
{
	if (CurrIB.Get() == pIB) OK;
	CurrIB = (CD3D11IndexBuffer*)pIB;
	CurrDirtyFlags.Set(GPU_Dirty_IB);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetRenderState(CRenderState* pState)
{
	if (CurrRS == pState || NewRS == pState) OK;
	NewRS = static_cast<CD3D11RenderState*>(pState);
	CurrDirtyFlags.Set(GPU_Dirty_RS);
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetRenderTarget(UPTR Index, CRenderTarget* pRT)
{
	if (Index >= CurrRT.GetCount()) FAIL;
	if (CurrRT[Index].Get() == pRT) OK;

#ifdef _DEBUG // Can't set the same RT to more than one slot
	if (pRT)
		for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
			if (CurrRT[i].Get() == pRT) FAIL;
#endif

	CurrRT[Index] = (CD3D11RenderTarget*)pRT;
	CurrDirtyFlags.Set(GPU_Dirty_RT);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetDepthStencilBuffer(CDepthStencilBuffer* pDS)
{
	if (CurrDS.Get() == pDS) OK;
	CurrDS = (CD3D11DepthStencilBuffer*)pDS;
	CurrDirtyFlags.Set(GPU_Dirty_DS);
	OK;
}
//---------------------------------------------------------------------

CRenderTarget* CD3D11GPUDriver::GetRenderTarget(UPTR Index) const
{
	return CurrRT[Index].Get();
}
//---------------------------------------------------------------------

CDepthStencilBuffer* CD3D11GPUDriver::GetDepthStencilBuffer() const
{
	return CurrDS.Get();
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::BindSRV(EShaderType ShaderType, UPTR SlotIndex, ID3D11ShaderResourceView* pSRV, CD3D11ConstantBuffer* pCB)
{
	if (SlotIndex >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) FAIL;

	auto Register = SlotIndex;
	if (MaxSRVSlotIndex < SlotIndex) MaxSRVSlotIndex = SlotIndex;
	SlotIndex |= (ShaderType << 16); // Encode shader type in a high word

	CSRVRecord* pSRVRec;
	IPTR DictIdx = CurrSRV.FindIndex(SlotIndex);
	if (DictIdx != INVALID_INDEX)
	{
		pSRVRec = &CurrSRV.ValueAt(DictIdx);
		if (pSRVRec->pSRV == pSRV) OK;

		// Free temporary buffer previously bound to this slot
		FreePendingTemporaryBuffer(pSRVRec->CB.Get(), ShaderType, Register);
	}
	else
	{
		pSRVRec = &CurrSRV.Add(SlotIndex);
	}

	pSRVRec->pSRV = pSRV;
	pSRVRec->CB = pCB;

	CurrDirtyFlags.Set(GPU_Dirty_SRV);
	ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_Resources + ShaderType));
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::BindConstantBuffer(EShaderType ShaderType, EUSMBufferType Type, U32 Register, CD3D11ConstantBuffer* pCBuffer)
{
	if (Type == USMBuffer_Constant)
	{
		if (Register >= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT) FAIL;

		const UPTR Index = Register + ((UPTR)ShaderType) * D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;

		if (CurrCB[Index] == pCBuffer) OK;

		// Free temporary buffer previously bound to this slot
		FreePendingTemporaryBuffer(CurrCB[Index].Get(), ShaderType, Register);

		CurrCB[Index] = pCBuffer;
		CurrDirtyFlags.Set(GPU_Dirty_CB);
		ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_CBuffers + ShaderType));

		OK;
	}
	else
	{
		ID3D11ShaderResourceView* pSRV = pCBuffer ? pCBuffer->GetD3DSRView() : nullptr;
		return BindSRV(ShaderType, Register, pSRV, pCBuffer);
	}
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::BindResource(EShaderType ShaderType, U32 Register, CD3D11Texture* pResource)
{
	ID3D11ShaderResourceView* pSRV = pResource ? pResource->GetD3DSRView() : nullptr;
	return BindSRV(ShaderType, Register, pSRV, nullptr);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::BindSampler(EShaderType ShaderType, U32 Register, CD3D11Sampler* pSampler)
{
	if (Register >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT) FAIL;

	const UPTR Index = Register + ((UPTR)ShaderType) * D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
	const CD3D11Sampler* pCurrSamp = CurrSS[Index].Get();
	if (pCurrSamp == pSampler) OK;
	if (pCurrSamp && pSampler && pCurrSamp->GetD3DSampler() == pSampler->GetD3DSampler()) OK;

	CurrSS[Index] = pSampler;
	CurrDirtyFlags.Set(GPU_Dirty_SS);
	ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_Samplers + ShaderType));

	OK;
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::UnbindSRV(EShaderType ShaderType, UPTR SlotIndex, ID3D11ShaderResourceView* pSRV)
{
	if (SlotIndex >= D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT) return;

	CSRVRecord* pSRVRec;
	IPTR DictIdx = CurrSRV.FindIndex(SlotIndex | (ShaderType << 16));
	if (DictIdx == INVALID_INDEX) return;

	pSRVRec = &CurrSRV.ValueAt(DictIdx);
	if (pSRVRec->pSRV != pSRV) return;

	// Free temporary buffer previously bound to this slot
	FreePendingTemporaryBuffer(pSRVRec->CB.Get(), ShaderType, SlotIndex);

	CurrSRV.RemoveAt(DictIdx);
	CurrDirtyFlags.Set(GPU_Dirty_SRV);
	ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_Resources + ShaderType));
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::UnbindConstantBuffer(EShaderType ShaderType, EUSMBufferType Type, U32 Register, CD3D11ConstantBuffer& CBuffer)
{
	if (Type == USMBuffer_Constant)
	{
		if (Register >= D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT) return;

		const UPTR Index = Register + ((UPTR)ShaderType) * D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
		if (CurrCB[Index] == &CBuffer)
		{
			// Free temporary buffer
			FreePendingTemporaryBuffer(CurrCB[Index], ShaderType, Register);

			CurrCB[Index] = nullptr;
			CurrDirtyFlags.Set(GPU_Dirty_CB);
			ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_CBuffers + ShaderType));
		}
	}
	else
	{
		UnbindSRV(ShaderType, Register, CBuffer.GetD3DSRView());
	}
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::UnbindResource(EShaderType ShaderType, U32 Register, CD3D11Texture& Resource)
{
	UnbindSRV(ShaderType, Register, Resource.GetD3DSRView());
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::UnbindSampler(EShaderType ShaderType, U32 Register, CD3D11Sampler& Sampler)
{
	if (Register >= D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT) return;
	const UPTR Index = Register + ((UPTR)ShaderType) * D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT;
	if (CurrSS[Index] == &Sampler)
	{
		CurrSS[Index] = nullptr;
		CurrDirtyFlags.Set(GPU_Dirty_SS);
		ShaderParamsDirtyFlags.Set(1 << (Shader_Dirty_Samplers + ShaderType));
	}
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::BeginFrame()
{
#ifdef DEM_STATS
	PrimitivesRendered = 0;
	DrawsRendered = 0;
#endif
	OK;
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::EndFrame()
{
#ifdef DEM_STATS
	CString RTString;
	for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		if (CurrRT[i].IsValidPtr())
			RTString += StringUtils::FromInt((int)CurrRT[i].Get());
	if (Core::CCoreServer::HasInstance())
	{
		CoreSrv->SetGlobal<int>(CString("Render_Primitives_") + RTString, PrimitivesRendered);
		CoreSrv->SetGlobal<int>(CString("Render_Draws_") + RTString, DrawsRendered);
	}
#endif

	/*
	//!!!may clear render targets in RP phases, this is not a solution!
	for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		SetRenderTarget(i, nullptr);
	SetDepthStencilBuffer(nullptr);
	*/
}
//---------------------------------------------------------------------

// Even if RTs and DS set are still dirty (not bound), clear operation can be performed.
void CD3D11GPUDriver::Clear(UPTR Flags, const vector4& ColorRGBA, float Depth, U8 Stencil)
{
	if (Flags & Clear_Color)
	{
		for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		{
			CD3D11RenderTarget* pRT = CurrRT[i].Get();
			if (pRT && pRT->IsValid())
				pD3DImmContext->ClearRenderTargetView(pRT->GetD3DRTView(), ColorRGBA.v);
		}
	}

	if (CurrDS.IsValidPtr() && ((Flags & Clear_Depth) || (Flags & Clear_Stencil)))
		ClearDepthStencilBuffer(*CurrDS.Get(), Flags, Depth, Stencil);
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA)
{
	if (!RT.IsValid()) return;
	CD3D11RenderTarget& D3D11RT = (CD3D11RenderTarget&)RT;
	pD3DImmContext->ClearRenderTargetView(D3D11RT.GetD3DRTView(), ColorRGBA.v);
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::ClearDepthStencilBuffer(CDepthStencilBuffer& DS, UPTR Flags, float Depth, U8 Stencil)
{
	if (!DS.IsValid()) return;
	CD3D11DepthStencilBuffer& D3D11DS = (CD3D11DepthStencilBuffer&)DS;

	UINT D3DFlags = 0;
	if (Flags & Clear_Depth) D3DFlags |= D3D11_CLEAR_DEPTH;

	DXGI_FORMAT Fmt = CD3D11DriverFactory::PixelFormatToDXGIFormat(D3D11DS.GetDesc().Format);
	if ((Flags & Clear_Stencil) && CD3D11DriverFactory::DXGIFormatStencilBits(Fmt) > 0)
		D3DFlags |= D3D11_CLEAR_STENCIL;

	if (D3DFlags)
		pD3DImmContext->ClearDepthStencilView(D3D11DS.GetD3DDSView(), D3DFlags, Depth, Stencil);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::InternalDraw(const CPrimitiveGroup& PrimGroup, bool Instanced, UPTR InstanceCount)
{
	n_assert_dbg(pD3DDevice && InstanceCount && (Instanced || InstanceCount == 1));

	if (CurrPT != PrimGroup.Topology)
	{
		D3D11_PRIMITIVE_TOPOLOGY D3DPrimType;
		switch (PrimGroup.Topology)
		{
			case Prim_PointList:	D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST; break;
			case Prim_LineList:		D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_LINELIST; break;
			case Prim_LineStrip:	D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
			case Prim_TriList:		D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
			case Prim_TriStrip:		D3DPrimType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
			default:				Sys::Error("CD3D11GPUDriver::Draw() -> Invalid primitive topology!"); FAIL;
		}

		pD3DImmContext->IASetPrimitiveTopology(D3DPrimType);
		CurrPT = PrimGroup.Topology;
	}

	if (CurrDirtyFlags.IsAny() && ApplyChanges(CurrDirtyFlags.GetMask()) != 0) FAIL;
	n_assert_dbg(CurrDirtyFlags.IsNotAll());

	if (PrimGroup.IndexCount > 0)
	{
		if (Instanced)
			pD3DImmContext->DrawIndexedInstanced(PrimGroup.IndexCount, InstanceCount, PrimGroup.FirstIndex, 0, 0);
		else
			pD3DImmContext->DrawIndexed(PrimGroup.IndexCount, PrimGroup.FirstIndex, 0);
	}
	else
	{
		if (Instanced)
			pD3DImmContext->DrawInstanced(PrimGroup.VertexCount, InstanceCount, PrimGroup.FirstVertex, 0);
		else
			pD3DImmContext->Draw(PrimGroup.VertexCount, PrimGroup.FirstVertex);
	}

#ifdef DEM_STATS
	UPTR PrimCount = (PrimGroup.IndexCount > 0) ? PrimGroup.IndexCount : PrimGroup.VertexCount;
	switch (CurrPT)
	{
		case Prim_PointList:	break;
		case Prim_LineList:		PrimCount >>= 1; break;
		case Prim_LineStrip:	--PrimCount; break;
		case Prim_TriList:		PrimCount /= 3; break;
		case Prim_TriStrip:		PrimCount -= 2; break;
		default:				PrimCount = 0; break;
	}
	PrimitivesRendered += InstanceCount * PrimCount;
	++DrawsRendered;
#endif

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
		const CD3D11RenderState* pNewRS = NewRS.Get();

		if (pNewRS)
		{
			CD3D11RenderState* pCurrRS = CurrRS.Get();

			UPTR CurrSigID = pCurrRS && pCurrRS->VS.IsValidPtr() ? pCurrRS->VS.Get()->GetInputSignatureID() : 0;
			UPTR NewSigID = pNewRS->VS.IsValidPtr() ? pNewRS->VS.Get()->GetInputSignatureID() : 0;

			if (!pCurrRS || NewSigID != CurrSigID) InputLayoutDirty = true;

			if (!pCurrRS || pNewRS->VS != pCurrRS->VS)
				pD3DImmContext->VSSetShader(pNewRS->VS.IsValidPtr() ? pNewRS->VS.Get()->GetD3DVertexShader() : nullptr, nullptr, 0);
			if (!pCurrRS || pNewRS->HS != pCurrRS->HS)
				pD3DImmContext->HSSetShader(pNewRS->HS.IsValidPtr() ? pNewRS->HS.Get()->GetD3DHullShader() : nullptr, nullptr, 0);
			if (!pCurrRS || pNewRS->DS != pCurrRS->DS)
				pD3DImmContext->DSSetShader(pNewRS->DS.IsValidPtr() ? pNewRS->DS.Get()->GetD3DDomainShader() : nullptr, nullptr, 0);
			if (!pCurrRS || pNewRS->GS != pCurrRS->GS)
				pD3DImmContext->GSSetShader(pNewRS->GS.IsValidPtr() ? pNewRS->GS.Get()->GetD3DGeometryShader() : nullptr, nullptr, 0);
			if (!pCurrRS || pNewRS->PS != pCurrRS->PS)
				pD3DImmContext->PSSetShader(pNewRS->PS.IsValidPtr() ? pNewRS->PS.Get()->GetD3DPixelShader() : nullptr, nullptr, 0);

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
			constexpr float EmptyFloats[4] = { 0 };
			pD3DImmContext->VSSetShader(nullptr, nullptr, 0);
			pD3DImmContext->HSSetShader(nullptr, nullptr, 0);
			pD3DImmContext->DSSetShader(nullptr, nullptr, 0);
			pD3DImmContext->GSSetShader(nullptr, nullptr, 0);
			pD3DImmContext->PSSetShader(nullptr, nullptr, 0);
			pD3DImmContext->OMSetBlendState(nullptr, EmptyFloats, 0xffffffff);
			pD3DImmContext->OMSetDepthStencilState(nullptr, 0);
			pD3DImmContext->RSSetState(nullptr);
		}

		CurrRS = std::move(NewRS);
		CurrDirtyFlags.Clear(GPU_Dirty_RS);
	}

	if (Update.Is(GPU_Dirty_VL) && CurrDirtyFlags.Is(GPU_Dirty_VL))
	{
		InputLayoutDirty = true;
		CurrDirtyFlags.Clear(GPU_Dirty_VL);
	}

	if (InputLayoutDirty)
	{
		UPTR InputSigID = CurrRS.IsValidPtr() && CurrRS->VS.IsValidPtr() ? CurrRS->VS->GetInputSignatureID() : 0;
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
				const CD3D11ConstantBuffer* pCB = CurrCB[Offset + i].Get();
				D3DBuffers[i] = pCB ? pCB->GetD3DBuffer() : nullptr;
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
				const CD3D11Sampler* pSamp = CurrSS[Offset + i].Get();
				D3DSamplers[i] = pSamp ? pSamp->GetD3DSampler() : nullptr;
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

	// This array is used to set pointer values into a D3D context. It can be force-casted to any type.
	// We define it on stack and therefore avoid a per-frame dynamic allocation or unsafe _malloca.
	// VS offsets & strides are stored in this buffer too.
	constexpr UPTR PtrArraySize = std::max(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT,
		std::max((int)(D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 3), D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT));
	void* PtrArray[PtrArraySize];

	if (Update.Is(GPU_Dirty_SRV) && CurrDirtyFlags.Is(GPU_Dirty_SRV))
	{
		if (CurrSRV.IsInAddMode()) CurrSRV.EndAdd();

		if (CurrSRV.GetCount())
		{
			const UPTR SRVArrayMemSize = (MaxSRVSlotIndex + 1) * sizeof(ID3D11ShaderResourceView*);
			ID3D11ShaderResourceView** ppSRV = (ID3D11ShaderResourceView**)PtrArray;
			::ZeroMemory(ppSRV, SRVArrayMemSize);

			UPTR CurrShaderType = ShaderType_Invalid;
			bool SkipShaderType = true;
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
					SRVSlot = (CurrKey & 0xffff);
				}

				if (AtTheEnd || ShaderType != CurrShaderType)
				{
					if (!SkipShaderType)
					{
						switch ((EShaderType)CurrShaderType)
						{
							case ShaderType_Vertex:
								pD3DImmContext->VSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								break;
							case ShaderType_Pixel:
								pD3DImmContext->PSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								break;
							case ShaderType_Geometry:
								pD3DImmContext->GSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								break;
							case ShaderType_Hull:
								pD3DImmContext->HSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								break;
							case ShaderType_Domain:
								pD3DImmContext->DSSetShaderResources(FirstSRVSlot, CurrSRVSlot - FirstSRVSlot + 1, ppSRV + FirstSRVSlot);
								break;
						};

						ShaderParamsDirtyFlags.Clear(1 << (Shader_Dirty_Resources + CurrShaderType));
					}

					if (AtTheEnd) break;

					if (!SkipShaderType)
					{
						::ZeroMemory(ppSRV, SRVArrayMemSize);
					}

					CurrShaderType = ShaderType;
					FirstSRVSlot = SRVSlot;

					SkipShaderType = ShaderParamsDirtyFlags.IsNot(1 << (Shader_Dirty_Resources + CurrShaderType));
				}

				if (SkipShaderType) continue;

				ppSRV[SRVSlot] = CurrSRV.ValueAt(i).pSRV;
				CurrSRVSlot = SRVSlot;
			}
		}

		CurrDirtyFlags.Clear(GPU_Dirty_SRV);
	}

	if (Update.Is(GPU_Dirty_VB) && CurrDirtyFlags.Is(GPU_Dirty_VB))
	{
		const UPTR MaxVBCount = CurrVB.GetCount();
		const UPTR PtrsSize = sizeof(ID3D11Buffer*) * MaxVBCount;
		const UPTR UINTsSize = sizeof(UINT) * MaxVBCount;
		char* pMem = (char*)PtrArray;
		n_assert(pMem);

		ID3D11Buffer** pVBs = (ID3D11Buffer**)pMem;
		UINT* pStrides = (UINT*)(pMem + PtrsSize);
		UINT* pOffsets = (UINT*)(pMem + PtrsSize + UINTsSize);

		//???PERF: skip all nullptr buffers prior to the first non-nullptr and all nullptr after the last non-nullptr and reduce count?
		for (UPTR i = 0; i < MaxVBCount; ++i)
		{
			CD3D11VertexBuffer* pVB = CurrVB[i].Get();
			if (pVB)
			{
				pVBs[i] = pVB->GetD3DBuffer();
				pStrides[i] = (UINT)pVB->GetVertexLayout()->GetVertexSizeInBytes();
				pOffsets[i] = (UINT)CurrVBOffset[i];
			}
			else
			{
				pVBs[i] = nullptr;
				pStrides[i] = 0;
				pOffsets[i] = 0;
			}
		}

		pD3DImmContext->IASetVertexBuffers(0, MaxVBCount, pVBs, pStrides, pOffsets);

		CurrDirtyFlags.Clear(GPU_Dirty_VB);
	}

	if (Update.Is(GPU_Dirty_IB) && CurrDirtyFlags.Is(GPU_Dirty_IB))
	{
		if (CurrIB.IsValidPtr())
		{
			const DXGI_FORMAT Fmt = CurrIB.Get()->GetIndexType() == Index_32 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
			pD3DImmContext->IASetIndexBuffer(CurrIB.Get()->GetD3DBuffer(), Fmt, 0);
		}
		else pD3DImmContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

		CurrDirtyFlags.Clear(GPU_Dirty_IB);
	}

	CD3D11RenderTarget* pValidRT = nullptr;

	// All render targets and a depth-stencil buffer are set atomically in D3D11
	if (Update.IsAny(GPU_Dirty_RT | GPU_Dirty_DS) && CurrDirtyFlags.IsAny(GPU_Dirty_RT | GPU_Dirty_DS))
	{
		ID3D11RenderTargetView** pRTV = (ID3D11RenderTargetView**)PtrArray;
		n_assert(pRTV);
		for (UPTR i = 0; i < CurrRT.GetCount(); ++i)
		{
			CD3D11RenderTarget* pRT = CurrRT[i].Get();
			if (pRT)
			{
				pRTV[i] = pRT->GetD3DRTView();
				pValidRT = pRT;
			}
			else pRTV[i] = nullptr;
		}

		ID3D11DepthStencilView* pDSV = CurrDS.IsValidPtr() ? CurrDS->GetD3DDSView() : nullptr;

		pD3DImmContext->OMSetRenderTargets(CurrRT.GetCount(), pRTV, pDSV);
		//OMSetRenderTargetsAndUnorderedAccessViews

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
		// 16 high bits of VPSRSetFlags are SR flags
		UPTR NumSR = VP_OR_SR_SET_FLAG_COUNT;
		for (; NumSR > 1; --NumSR)
			if (VPSRSetFlags.Is(1 << (VP_OR_SR_SET_FLAG_COUNT + NumSR - 1))) break;

		pD3DImmContext->RSSetScissorRects(NumSR, NumSR ? CurrSR : nullptr);

		CurrDirtyFlags.Clear(GPU_Dirty_SR);
	}

	return Errors;
}
//---------------------------------------------------------------------

// Gets or creates an actual layout for the given vertex layout and shader input signature
ID3D11InputLayout* CD3D11GPUDriver::GetD3DInputLayout(CD3D11VertexLayout& VertexLayout, UPTR ShaderInputSignatureID, const Data::CBuffer* pSignature)
{
	if (!ShaderInputSignatureID) return nullptr;

	ID3D11InputLayout* pLayout = VertexLayout.GetD3DInputLayout(ShaderInputSignatureID);
	if (pLayout) return pLayout;

	const D3D11_INPUT_ELEMENT_DESC* pD3DDesc = VertexLayout.GetCachedD3DLayoutDesc();
	if (!pD3DDesc) return nullptr;

	if (!pSignature)
	{
		pSignature = _DriverFactory->FindShaderInputSignature(ShaderInputSignatureID);
		if (!pSignature) return nullptr;
	}

	if (FAILED(pD3DDevice->CreateInputLayout(pD3DDesc, VertexLayout.GetComponentCount(), pSignature->GetPtr(), pSignature->GetSize(), &pLayout)))
		return nullptr;

	n_verify_dbg(VertexLayout.AddLayoutObject(ShaderInputSignatureID, pLayout));

	return pLayout;
}
//---------------------------------------------------------------------

// Creates transparent user object
PVertexLayout CD3D11GPUDriver::CreateVertexLayout(const CVertexComponent* pComponents, UPTR Count)
{
	const UPTR MAX_VERTEX_COMPONENTS = 32;

	if (!pComponents || !Count || Count > MAX_VERTEX_COMPONENTS) return nullptr;

	CStrID Signature = CVertexLayout::BuildSignature(pComponents, Count);

	IPTR Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx).Get();

	UPTR MaxVertexStreams = GetMaxVertexStreams();

	D3D11_INPUT_ELEMENT_DESC DeclData[MAX_VERTEX_COMPONENTS] = { 0 };
	for (UPTR i = 0; i < Count; ++i)
	{
		const CVertexComponent& Component = pComponents[i];

		UPTR StreamIndex = Component.Stream;
		if (StreamIndex >= MaxVertexStreams) return nullptr;

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
			default:				return nullptr;
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
			default:					return nullptr;
		}

		DeclElement.InputSlot = StreamIndex;
		DeclElement.AlignedByteOffset =
			(Component.OffsetInVertex == DEM_VERTEX_COMPONENT_OFFSET_DEFAULT) ? D3D11_APPEND_ALIGNED_ELEMENT : Component.OffsetInVertex;
		if (Component.PerInstanceData)
		{
			DeclElement.InputSlotClass = D3D11_INPUT_PER_INSTANCE_DATA;
			DeclElement.InstanceDataStepRate = 1;
		}
		else
		{
			DeclElement.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			DeclElement.InstanceDataStepRate = 0;
		}
	}

	PD3D11VertexLayout Layout = n_new(CD3D11VertexLayout);
	if (!Layout->Create(pComponents, Count, DeclData)) return nullptr;

	VertexLayouts.Add(Signature, Layout);

	return Layout.Get();
}
//---------------------------------------------------------------------

PVertexBuffer CD3D11GPUDriver::CreateVertexBuffer(CVertexLayout& VertexLayout, UPTR VertexCount, UPTR AccessFlags, const void* pData)
{
	if (!pD3DDevice || !VertexCount || !VertexLayout.GetVertexSizeInBytes()) return nullptr;

	D3D11_USAGE Usage;
	UINT CPUAccess;
	if (!GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess)) return nullptr;

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = Usage;
	Desc.ByteWidth = VertexCount * VertexLayout.GetVertexSizeInBytes();
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Desc.CPUAccessFlags = CPUAccess;
	Desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
	D3D11_SUBRESOURCE_DATA InitData;
	if (pData)
	{
		InitData.pSysMem = pData;
		pInitData = &InitData;
	}

	ID3D11Buffer* pD3DBuf = nullptr;
	if (FAILED(pD3DDevice->CreateBuffer(&Desc, pInitData, &pD3DBuf))) return nullptr;

	PD3D11VertexBuffer VB = n_new(CD3D11VertexBuffer);
	if (!VB->Create(VertexLayout, pD3DBuf))
	{
		pD3DBuf->Release();
		return nullptr;
	}

	return VB.Get();
}
//---------------------------------------------------------------------

PIndexBuffer CD3D11GPUDriver::CreateIndexBuffer(EIndexType IndexType, UPTR IndexCount, UPTR AccessFlags, const void* pData)
{
	if (!pD3DDevice || !IndexCount) return nullptr;

	D3D11_USAGE Usage;
	UINT CPUAccess;
	if (!GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess)) return nullptr;

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = Usage;
	Desc.ByteWidth = IndexCount * (UPTR)IndexType;
	Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	Desc.CPUAccessFlags = CPUAccess;
	Desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
	D3D11_SUBRESOURCE_DATA InitData;
	if (pData)
	{
		InitData.pSysMem = pData;
		pInitData = &InitData;
	}

	ID3D11Buffer* pD3DBuf = nullptr;
	if (FAILED(pD3DDevice->CreateBuffer(&Desc, pInitData, &pD3DBuf))) return nullptr;

	PD3D11IndexBuffer IB = n_new(CD3D11IndexBuffer);
	if (!IB->Create(IndexType, pD3DBuf))
	{
		pD3DBuf->Release();
		return nullptr;
	}

	return IB.Get();
}
//---------------------------------------------------------------------

//!!!shader reflection doesn't return StructuredBuffer element count! So we must pass it here and ignore parameter for other buffers!
//!!!or we must determine buffer size in shader comments(annotations?) / in effect desc!
PD3D11ConstantBuffer CD3D11GPUDriver::InternalCreateConstantBuffer(EUSMBufferType Type, U32 Size, UPTR AccessFlags, const CConstantBuffer* pData, bool Temporary)
{
	D3D11_USAGE Usage;
	UINT CPUAccess;
	if (!GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess)) return nullptr;

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = Usage;
	Desc.CPUAccessFlags = CPUAccess;

	UPTR TotalSize = Size; // * ElementCount; //!!!for StructuredBuffer! or precompute in metadata?
	if (Type == USMBuffer_Constant)
	{
		UPTR ElementCount = (TotalSize + 15) >> 4;
		if (ElementCount > D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT) return nullptr;
		Desc.ByteWidth = ElementCount << 4;
		Desc.BindFlags = (Usage == D3D11_USAGE_STAGING) ? 0 : D3D11_BIND_CONSTANT_BUFFER;
	}
	else
	{
		Desc.ByteWidth = TotalSize;
		Desc.BindFlags = (Usage == D3D11_USAGE_STAGING) ? 0 : D3D11_BIND_SHADER_RESOURCE;
	}

	if (Type == USMBuffer_Structured)
	{
		Desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		Desc.StructureByteStride = Size;
	}
	else
	{
		Desc.MiscFlags = 0;
		Desc.StructureByteStride = 0;
	}

	D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
	D3D11_SUBRESOURCE_DATA InitData;
	if (pData)
	{
		if (!pData->IsA<CD3D11ConstantBuffer>()) return nullptr;
		const CD3D11ConstantBuffer* pDataCB11 = (const CD3D11ConstantBuffer*)pData;
		InitData.pSysMem = pDataCB11->GetRAMCopy();
		pInitData = &InitData;
	}

	ID3D11Buffer* pD3DBuf = nullptr;
	if (FAILED(pD3DDevice->CreateBuffer(&Desc, pInitData, &pD3DBuf))) return nullptr;

	ID3D11ShaderResourceView* pSRV = nullptr;
	if (Desc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		SRVDesc.Buffer.FirstElement = 0;
		SRVDesc.Buffer.NumElements = TotalSize / (4 * sizeof(float));
		if (FAILED(pD3DDevice->CreateShaderResourceView(pD3DBuf, &SRVDesc, &pSRV)))
		{
			pD3DBuf->Release();
			return nullptr;
		}
	}

	PD3D11ConstantBuffer CB = n_new(CD3D11ConstantBuffer(pD3DBuf, pSRV, Temporary));
	if (!CB->IsValid()) return nullptr;

	//???or add manual control / some flag in a metadata?
	if (Usage != D3D11_USAGE_IMMUTABLE)
	{
		if (!CB->CreateRAMCopy()) return nullptr;
	}

	return CB;
}
//---------------------------------------------------------------------

//!!!shader reflection doesn't return StructuredBuffer element count! So we must pass it here and ignore parameter for other buffers!
//!!!or we must determine buffer size in shader comments(annotations?) / in effect desc!
PConstantBuffer CD3D11GPUDriver::CreateConstantBuffer(IConstantBufferParam& Param, UPTR AccessFlags, const CConstantBuffer* pData)
{
	auto pUSMParam = Param.As<CUSMConstantBufferParam>();
	if (!pD3DDevice || !pUSMParam || !pUSMParam->GetSize()) return nullptr;

	return InternalCreateConstantBuffer(pUSMParam->GetType(), pUSMParam->GetSize(), AccessFlags, pData, false);
}
//---------------------------------------------------------------------

PConstantBuffer CD3D11GPUDriver::CreateTemporaryConstantBuffer(IConstantBufferParam& Param)
{
	auto pUSMParam = Param.As<CUSMConstantBufferParam>();
	if (!pD3DDevice || !pUSMParam || !pUSMParam->GetSize()) return nullptr;

	// We create temporary buffers sized by powers of 2, to make reuse easier (the same
	// principle as for a small allocator). 16 bytes is the smallest possible buffer.
	UPTR NextPow2Size = std::max<U32>(16, NextPow2(pUSMParam->GetSize())/* * ElementCount; //!!!for StructuredBuffer!*/); 
	CDict<UPTR, CTmpCB*>& BufferPool = 
		pUSMParam->GetType() == USMBuffer_Structured ? TmpStructuredBuffers :
		(pUSMParam->GetType() == USMBuffer_Texture ? TmpTextureBuffers : TmpConstantBuffers);

	IPTR Idx = BufferPool.FindIndex(NextPow2Size);
	if (Idx != INVALID_INDEX)
	{
		CTmpCB* pHead = BufferPool.ValueAt(Idx);
		if (pHead)
		{
			BufferPool.ValueAt(Idx) = pHead->pNext;
			PConstantBuffer CB = pHead->CB.Get();
			TmpCBPool.Destroy(pHead);
			return CB;
		}
	}

	return InternalCreateConstantBuffer(pUSMParam->GetType(), NextPow2Size, Access_CPU_Write | Access_GPU_Read, nullptr, true);
}
//---------------------------------------------------------------------

// NB: when buffer is freed, we can reuse it immediately due to dynamic CB renaming, see:
// https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0
void CD3D11GPUDriver::FreeTemporaryConstantBuffer(CConstantBuffer& CBuffer)
{
	CD3D11ConstantBuffer& CB11 = (CD3D11ConstantBuffer&)CBuffer;

	UPTR BufferSize = CB11.GetSizeInBytes();
#ifdef _DEBUG
	n_assert(BufferSize == NextPow2(CB11.GetSizeInBytes()));
#endif

	CTmpCB* pNewNode = TmpCBPool.Construct();
	pNewNode->CB = &CB11;

	if (IsConstantBufferBound(&CB11))
	{
		pNewNode->pNext = pPendingCBHead;
		pPendingCBHead = pNewNode;
	}
	else
	{
		EUSMBufferType Type = CB11.GetType();
		CDict<UPTR, CTmpCB*>& BufferPool = 
			Type == USMBuffer_Structured ? TmpStructuredBuffers :
			(Type == USMBuffer_Texture ? TmpTextureBuffers : TmpConstantBuffers);
		IPTR Idx = BufferPool.FindIndex(BufferSize);
		if (Idx != INVALID_INDEX)
		{
			pNewNode->pNext = BufferPool.ValueAt(Idx);
			BufferPool.ValueAt(Idx) = pNewNode;
		}
		else
		{
			pNewNode->pNext = nullptr;
			BufferPool.Add(BufferSize, pNewNode);
		}
	}
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::IsConstantBufferBound(const CD3D11ConstantBuffer* pCBuffer, EShaderType ExceptStage, UPTR ExceptSlot)
{
	if (!pCBuffer) FAIL;

	if (pCBuffer->GetType() == USMBuffer_Constant)
	{
		if (ExceptStage == ShaderType_Invalid)
		{
			for (UPTR i = 0; i < CurrCB.GetCount(); ++i)
				if (CurrCB[i].Get() == pCBuffer) OK;
		}
		else
		{
			UPTR ExceptIndex = ExceptSlot + ((UPTR)ExceptStage) * D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT;
			for (UPTR i = 0; i < CurrCB.GetCount(); ++i)
			{
				if (i == ExceptIndex) continue;
				if (CurrCB[i].Get() == pCBuffer) OK;
			}
		}
	}
	else
	{
		if (ExceptStage == ShaderType_Invalid)
		{
			for (UPTR i = 0; i < CurrSRV.GetCount(); ++i)
				if (CurrSRV.ValueAt(i).CB.Get() == pCBuffer) OK;
		}
		else
		{
			UPTR ExceptIndex = ExceptSlot | (ExceptStage << 16);
			for (UPTR i = 0; i < CurrSRV.GetCount(); ++i)
			{
				if (CurrSRV.KeyAt(i) == ExceptIndex) continue;
				if (CurrSRV.ValueAt(i).CB.Get() == pCBuffer) OK;
			}
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::FreePendingTemporaryBuffer(const CD3D11ConstantBuffer* pCBuffer, EShaderType Stage, UPTR Slot)
{
	// Check whether there are pending buffers, this buffer is temporary and this buffer is not bound
	if (!pPendingCBHead || !pCBuffer || !pCBuffer->IsTemporary() || IsConstantBufferBound(pCBuffer, Stage, Slot)) return;

	EUSMBufferType Type = pCBuffer->GetType();
	CDict<UPTR, CTmpCB*>& BufferPool = 
		Type == USMBuffer_Structured ? TmpStructuredBuffers :
		(Type == USMBuffer_Texture ? TmpTextureBuffers : TmpConstantBuffers);

	CTmpCB* pPrevNode = nullptr;
	CTmpCB* pCurrNode = pPendingCBHead;
	while (pCurrNode)
	{
		if (pCurrNode->CB == pCBuffer)
		{
			if (pPrevNode) pPrevNode->pNext = pCurrNode->pNext;
			else pPendingCBHead = pCurrNode->pNext;

			UPTR BufferSize = pCBuffer->GetSizeInBytes();
			n_assert_dbg(BufferSize == NextPow2(pCBuffer->GetSizeInBytes()));
			IPTR Idx = BufferPool.FindIndex(BufferSize);
			if (Idx != INVALID_INDEX)
			{
				pCurrNode->pNext = BufferPool.ValueAt(Idx);
				BufferPool.ValueAt(Idx) = pCurrNode;
			}
			else
			{
				pCurrNode->pNext = nullptr;
				BufferPool.Add(BufferSize, pCurrNode);
			}

			break;
		}

		pPrevNode = pCurrNode;
		pCurrNode = pCurrNode->pNext;
	}
}
//---------------------------------------------------------------------

// if Data->MipDataProvided, order is ArrayElement[0] { Mip[0] ... Mip[N] } ... ArrayElement[M] { Mip[0] ... Mip[N] },
// else order is ArrayElement[0] { Mip[0] } ... ArrayElement[M] { Mip[0] }, where Mip[0] is an original data.
PTexture CD3D11GPUDriver::CreateTexture(PTextureData Data, UPTR AccessFlags)
{
	if (!pD3DDevice || !Data || !Data->Desc.Width || !Data->Desc.Height) return nullptr;

	const CTextureDesc& Desc = Data->Desc;

	if (Desc.Type != Texture_1D && Desc.Type != Texture_2D && Desc.Type != Texture_Cube && Desc.Type != Texture_3D)
	{
		Sys::Error("CD3D11GPUDriver::CreateTexture() > Unknown texture type %d\n", Desc.Type);
		return nullptr;
	}

	const void* pData = Data->Data ? Data->Data->GetConstPtr() : nullptr;
	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);
	UPTR QualityLvlCount = 0;
	if (Desc.MSAAQuality != MSAA_None)
	{
		if (FAILED(pD3DDevice->CheckMultisampleQualityLevels(DXGIFormat, (int)Desc.MSAAQuality, &QualityLvlCount)) || !QualityLvlCount)
			return nullptr;
		pData = nullptr; // MSAA resources can't be initialized with data
	}

	D3D11_USAGE Usage;
	UINT CPUAccess;
	if (!GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess)) return nullptr;

	UPTR MipLevels = Desc.MipLevels;
	UPTR ArraySize = (Desc.Type == Texture_3D) ? 1 : ((Desc.Type == Texture_Cube) ? 6 * Desc.ArraySize : Desc.ArraySize);
	UPTR MiscFlags = 0; //???if (MipLevels != 1) D3D11_RESOURCE_MISC_RESOURCE_CLAMP for ID3D11DeviceContext::SetResourceMinLOD()
	UPTR BindFlags = (Usage != D3D11_USAGE_STAGING) ? D3D11_BIND_SHADER_RESOURCE : 0;

	// Dynamic SRV: The resource can only be created with a single subresource.
	// The resource cannot be a texture array. The resource cannot be a mipmap chain. (c) Docs
	if (Usage == D3D11_USAGE_DYNAMIC && (MipLevels != 1 || ArraySize != 1))
	{
#ifdef _DEBUG
		Sys::DbgOut("CD3D11GPUDriver::CreateTexture() > Dynamic texture requested %d mips and %d array slices. D3D11 requires 1 for both. Values are changed to 1.\n", MipLevels, ArraySize);
#endif
		MipLevels = 1;
		ArraySize = 1;
	}

	D3D11_SUBRESOURCE_DATA* pInitData = nullptr;
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
		char* pGeneratedMips = nullptr;
		if (!Data->MipDataProvided && MipLevels > 1)
		{
			NOT_IMPLEMENTED;
			//!!!generate mips by DirectXTex code or smth like that!
#ifdef _DEBUG
			if (!pGeneratedMips)
				Sys::DbgOut("CD3D11GPUDriver::CreateTexture() > Mipmaps are not generated\n");
#endif
		}

		pInitData = n_new_array(D3D11_SUBRESOURCE_DATA, MipLevels * ArraySize);
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
				if (Data->MipDataProvided)
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
					pCurrInitData->pSysMem = nullptr;
					pCurrInitData->SysMemPitch = 0;
					pCurrInitData->SysMemSlicePitch = 0;
					++pCurrInitData;
				}
			}
		}
	}

	ID3D11Resource* pTexRsrc = nullptr;
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

		ID3D11Texture1D* pD3DTex = nullptr;
		HRESULT hr = pD3DDevice->CreateTexture1D(&D3DDesc, pInitData, &pD3DTex);
		SAFE_DELETE_ARRAY(pInitData);
		if (FAILED(hr)) return nullptr;
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
			D3DDesc.SampleDesc.Count = static_cast<int>(Desc.MSAAQuality);
			D3DDesc.SampleDesc.Quality = QualityLvlCount - 1; // Can use predefined D3D11_STANDARD_MULTISAMPLE_PATTERN, D3D11_CENTER_MULTISAMPLE_PATTERN
		}

		ID3D11Texture2D* pD3DTex = nullptr;
		HRESULT hr = pD3DDevice->CreateTexture2D(&D3DDesc, pInitData, &pD3DTex);
		SAFE_DELETE_ARRAY(pInitData);
		if (FAILED(hr)) return nullptr;
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

		ID3D11Texture3D* pD3DTex = nullptr;
		HRESULT hr = pD3DDevice->CreateTexture3D(&D3DDesc, pInitData, &pD3DTex);
		SAFE_DELETE_ARRAY(pInitData);
		if (FAILED(hr)) return nullptr;
		pTexRsrc = pD3DTex;
	}
	else
	{
		SAFE_DELETE_ARRAY(pInitData);
		return nullptr;
	}

	ID3D11ShaderResourceView* pSRV = nullptr;
	if ((BindFlags & D3D11_BIND_SHADER_RESOURCE) &&
		FAILED(pD3DDevice->CreateShaderResourceView(pTexRsrc, nullptr, &pSRV)))
	{
		pTexRsrc->Release();
		return nullptr;
	}

	PD3D11Texture Tex = n_new(CD3D11Texture);
	if (Tex.IsNullPtr()) return nullptr;

	if (!Tex->Create(Data, Usage, AccessFlags, pTexRsrc, pSRV))
	{
		pSRV->Release();
		pTexRsrc->Release();
		return nullptr;
	}

	return Tex.Get();
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
	D3DDesc.MaxAnisotropy = std::clamp<unsigned int>(Desc.MaxAnisotropy, 1, 16);
	D3DDesc.MaxLOD = Desc.CoarsestMipMapLOD;
	D3DDesc.MinLOD = Desc.FinestMipMapLOD;
	D3DDesc.MipLODBias = Desc.MipMapLODBias;

	ID3D11SamplerState* pD3DSamplerState = nullptr;
	if (FAILED(pD3DDevice->CreateSamplerState(&D3DDesc, &pD3DSamplerState))) return nullptr;

	// Since sampler creation should be load-time, it is not performance critical.
	// We can omit it and allow to create duplicate samplers, but maintaining uniquity
	// serves both for memory saving and early exits on redundant binding.
	for (UPTR i = 0; i < Samplers.GetCount(); ++i)
	{
		CD3D11Sampler* pSamp = Samplers[i].Get();
		if (pSamp->GetD3DSampler() == pD3DSamplerState) return pSamp; //???release pD3DSamplerState?
	}

	PD3D11Sampler Samp = n_new(CD3D11Sampler);
	if (!Samp->Create(pD3DSamplerState))
	{
		pD3DSamplerState->Release();
		return nullptr;
	}

	Samplers.Add(Samp);

	return Samp.Get();
}
//---------------------------------------------------------------------

//???allow arrays? allow 3D and cubes? will need RT.Create or CreateRenderTarget(Texture, SurfaceLocation)
PRenderTarget CD3D11GPUDriver::CreateRenderTarget(const CRenderTargetDesc& Desc)
{
	DXGI_FORMAT Fmt = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);

	UPTR QualityLvlCount = 0;
	if (Desc.MSAAQuality != MSAA_None)
		if (FAILED(pD3DDevice->CheckMultisampleQualityLevels(Fmt, (int)Desc.MSAAQuality, &QualityLvlCount)) || !QualityLvlCount) return nullptr;

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

	ID3D11Texture2D* pTexture = nullptr;
	if (FAILED(pD3DDevice->CreateTexture2D(&D3DDesc, nullptr, &pTexture))) return nullptr;
	
	ID3D11RenderTargetView* pRTV = nullptr;
	if (FAILED(pD3DDevice->CreateRenderTargetView(pTexture, nullptr, &pRTV)))
	{
		pTexture->Release();
		return nullptr;
	}

	ID3D11ShaderResourceView* pSRV = nullptr;
	if (Desc.UseAsShaderInput)
	{
		if (FAILED(pD3DDevice->CreateShaderResourceView(pTexture, nullptr, &pSRV)))
		{
			pRTV->Release();
			pTexture->Release();
			return nullptr;
		}
	}

	pTexture->Release();

	PD3D11RenderTarget RT = n_new(CD3D11RenderTarget);
	if (!RT->Create(pRTV, pSRV))
	{
		if (pSRV) pSRV->Release();
		pRTV->Release();
		return nullptr;
	}
	return RT.Get();
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
		if (FAILED(pD3DDevice->CheckMultisampleQualityLevels(DSVFmt, (int)Desc.MSAAQuality, &QualityLvlCount)) || !QualityLvlCount) return nullptr;

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

	ID3D11Texture2D* pTexture = nullptr;
	if (FAILED(pD3DDevice->CreateTexture2D(&D3DDesc, nullptr, &pTexture))) return nullptr;

	D3D11_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	if (Desc.MSAAQuality == MSAA_None)
	{
		DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice = 0;
	}
	else DSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	DSVDesc.Format = DSVFmt;
	DSVDesc.Flags = 0; // D3D11_DSV_READ_ONLY_DEPTH, D3D11_DSV_READ_ONLY_STENCIL

	ID3D11DepthStencilView* pDSV = nullptr;
	if (FAILED(pD3DDevice->CreateDepthStencilView(pTexture, &DSVDesc, &pDSV)))
	{
		pTexture->Release();
		return nullptr;
	}

	ID3D11ShaderResourceView* pSRV = nullptr;
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
			return nullptr;
		}
	}

	pTexture->Release();

	PD3D11DepthStencilBuffer DS = n_new(CD3D11DepthStencilBuffer);
	if (!DS->Create(pDSV, pSRV))
	{
		if (pSRV) pSRV->Release();
		pDSV->Release();
		return nullptr;
	}
	return DS.Get();
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

	ID3D11RasterizerState* pRState = nullptr;
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

	ID3D11DepthStencilState* pDSState = nullptr;
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

	ID3D11BlendState* pBState = nullptr;
	if (FAILED(pD3DDevice->CreateBlendState(&BDesc, &pBState))) goto ProcessFailure;

	// Since render state creation should be load-time, it is not performance critical. If we
	// skip this and create new CRenderState, sorting will consider them as different state sets.
	for (UPTR i = 0; i < RenderStates.GetCount(); ++i)
	{
		CD3D11RenderState* pRS = RenderStates[i].Get();
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
		RS->VS = (CD3D11Shader*)Desc.VertexShader.Get();
		RS->PS = (CD3D11Shader*)Desc.PixelShader.Get();
		RS->GS = (CD3D11Shader*)Desc.GeometryShader.Get();
		RS->HS = (CD3D11Shader*)Desc.HullShader.Get();
		RS->DS = (CD3D11Shader*)Desc.DomainShader.Get();
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

		return RS;
	}

ProcessFailure:

	if (pRState) pRState->Release();
	if (pDSState) pDSState->Release();
	if (pBState) pBState->Release();

	return nullptr;
}
//---------------------------------------------------------------------

PShader CD3D11GPUDriver::CreateShader(IO::CStream& Stream, CShaderLibrary* pLibrary, bool LoadParamTable)
{
	IO::CBinaryReader R(Stream);

	U32 ShaderFormatCode;
	if (!R.Read(ShaderFormatCode) || !SupportsShaderFormat(ShaderFormatCode)) return nullptr;

	U32 MinFeatureLevel;
	if (!R.Read(MinFeatureLevel) || static_cast<EGPUFeatureLevel>(MinFeatureLevel) > FeatureLevel) return nullptr;

	U8 ShaderTypeCode;
	if (!R.Read(ShaderTypeCode)) return nullptr;
	Render::EShaderType ShaderType = static_cast<EShaderType>(ShaderTypeCode);

	U32 MetadataSize;
	if (!R.Read(MetadataSize)) return nullptr;

	U32 InputSignatureID;
	if (!R.Read(InputSignatureID)) return nullptr;
	MetadataSize -= sizeof(U32);

	U64 RequiresFlags;
	if (!R.Read(RequiresFlags)) return nullptr;
	MetadataSize -= sizeof(U64);

	// TODO: check GPU against RequiresFlags

	PShaderParamTable Params;
	if (LoadParamTable)
	{
		Params = LoadShaderParamTable(ShaderFormatCode, Stream);
		if (!Params) return nullptr;
	}
	else
	{
		if (!Stream.Seek(MetadataSize, IO::Seek_Current)) return nullptr;
	}

	const UPTR BinarySize = static_cast<UPTR>(Stream.GetSize() - Stream.GetPosition());
	if (!BinarySize) return nullptr;

	Data::CBuffer Data(BinarySize);
	if (Stream.Read(Data.GetPtr(), BinarySize) != BinarySize) return nullptr;

	PD3D11Shader Shader;

	switch (ShaderType)
	{
		case ShaderType_Vertex:
		{
			ID3D11VertexShader* pVS = nullptr;
			if (FAILED(pD3DDevice->CreateVertexShader(Data.GetPtr(), BinarySize, nullptr, &pVS))) return nullptr;
			Shader = n_new(CD3D11Shader(pVS, InputSignatureID, Params));
			break;
		}
		case ShaderType_Pixel:
		{
			ID3D11PixelShader* pPS = nullptr;
			if (FAILED(pD3DDevice->CreatePixelShader(Data.GetPtr(), BinarySize, nullptr, &pPS))) return nullptr;
			Shader = n_new(CD3D11Shader(pPS, Params));
			break;
		}
		case ShaderType_Geometry:
		{
			//???need stream output? or separate method? or separate shader type?
			ID3D11GeometryShader* pGS = nullptr;
			if (FAILED(pD3DDevice->CreateGeometryShader(Data.GetPtr(), BinarySize, nullptr, &pGS))) return nullptr;
			Shader = n_new(CD3D11Shader(pGS, InputSignatureID, Params));
			break;
		}
		case ShaderType_Hull:
		{
			ID3D11HullShader* pHS = nullptr;
			if (FAILED(pD3DDevice->CreateHullShader(Data.GetPtr(), BinarySize, nullptr, &pHS))) return nullptr;
			Shader = n_new(CD3D11Shader(pHS, Params));
			break;
		}
		case ShaderType_Domain:
		{
			ID3D11DomainShader* pDS = nullptr;
			if (FAILED(pD3DDevice->CreateDomainShader(Data.GetPtr(), BinarySize, nullptr, &pDS))) return nullptr;
			Shader = n_new(CD3D11Shader(pDS, Params));
			break;
		}
		default: return nullptr;
	};

	if (!Shader->IsValid()) return nullptr;

	if (ShaderType == Render::ShaderType_Vertex || ShaderType == Render::ShaderType_Geometry)
	{
		// Vertex shader input comes from input assembler stage (IA). In D3D10 and later
		// input layouts are created from VS input signatures (or at least are validated
		// against them). Input layout, once created, can be reused with any vertex shader
		// with the same input signature.

		/*
		// If no signature, can load this shader itself with some unique ID, for example negative.
		// This must never happen for shaders built in DEM, so we omit handling this case.
		if (InputSignatureID == 0)
		{
			if (!_DriverFactory->RegisterShaderInputSignature(InputSignatureID, std::move(Data))) return nullptr;
		}
		*/

		if (!_DriverFactory->FindShaderInputSignature(InputSignatureID))
		{
			if (pLibrary)
			{
				Data::PBuffer Buf = pLibrary->CopyRawData(InputSignatureID);
				if (!Buf) return nullptr;
				if (!_DriverFactory->RegisterShaderInputSignature(InputSignatureID, std::move(*Buf))) return nullptr;
			}
			else
			{
				//!!!DBG TMP!

				std::string FileName("Data:shaders/d3d_usm/sig/");
				FileName += std::to_string(InputSignatureID);
				FileName += ".sig";

				Data::CBuffer Buffer;
				IO::PStream File = IOSrv->CreateStream(FileName.c_str());
				if (!File || !File->Open(IO::SAM_READ, IO::SAP_SEQUENTIAL)) return nullptr;
				const UPTR FileSize = static_cast<UPTR>(File->GetSize());
				Buffer.Reserve(FileSize);
				Buffer.Trim(File->Read(Buffer.GetPtr(), FileSize));
				if (Buffer.GetSize() != FileSize) return nullptr;

				if (!_DriverFactory->RegisterShaderInputSignature(InputSignatureID, std::move(Buffer))) return nullptr;
			}
		}
	}

	Data.Clear();

	return Shader.Get();
}
//---------------------------------------------------------------------

PShaderParamTable CD3D11GPUDriver::LoadShaderParamTable(uint32_t ShaderFormatCode, IO::CStream& Stream)
{
	if (!SupportsShaderFormat(ShaderFormatCode)) return nullptr;

	IO::CBinaryReader R(Stream);

	U32 Count;

	if (!R.Read(Count)) return nullptr;
	std::vector<PConstantBufferParam> Buffers(Count);
	for (auto& BufferPtr : Buffers)
	{
		auto Name = R.Read<CStrID>();
		auto Register = R.Read<U32>();
		auto Size = R.Read<U32>();

		EUSMBufferType Type;
		switch (Register >> 30)
		{
			case 0:		Type = USMBuffer_Constant; break;
			case 1:		Type = USMBuffer_Texture; break;
			case 2:		Type = USMBuffer_Structured; break;
			default:	return nullptr;
		};

		Register &= 0x3fffffff; // Clear bits 30 and 31

		BufferPtr = n_new(CUSMConstantBufferParam(Name, 0, Type, Register, Size));
	}

	if (!R.Read(Count)) return nullptr;
	std::vector<PShaderStructureInfo> Structs(Count);

	// Precreate for valid referencing (see StructIndex)
	for (auto& StructPtr : Structs)
		StructPtr = n_new(CShaderStructureInfo);

	for (auto& StructPtr : Structs)
	{
		auto& Struct = *StructPtr;

		std::vector<PShaderConstantInfo> Members(R.Read<U32>());
		for (auto& MemberPtr : Members)
		{
			MemberPtr = n_new(CUSMConstantInfo());
			auto& Member = *static_cast<CUSMConstantInfo*>(MemberPtr.Get());

			if (!R.Read(Member.Name)) return nullptr;

			U32 StructIndex;
			if (!R.Read(StructIndex)) return nullptr;
			if (StructIndex != static_cast<U32>(-1))
				Member.Struct = Structs[StructIndex];

			Member.Type = static_cast<EUSMConstType>(R.Read<U8>());
			if (!R.Read(Member.LocalOffset)) return nullptr;
			if (!R.Read(Member.ElementStride)) return nullptr;
			if (!R.Read(Member.ElementCount)) return nullptr;
			if (!R.Read(Member.Columns)) return nullptr;
			if (!R.Read(Member.Rows)) return nullptr;
			if (!R.Read(Member.Flags)) return nullptr;

			Member.CalculateCachedValues();
		}

		Struct.SetMembers(std::move(Members));
	}

	if (!R.Read(Count)) return nullptr;
	std::vector<CShaderConstantParam> Consts(Count);
	for (auto& Const : Consts)
	{
		U8 ShaderTypeMask;
		if (!R.Read(ShaderTypeMask)) return nullptr;

		PUSMConstantInfo Info = n_new(CUSMConstantInfo());

		if (!R.Read(Info->Name)) return nullptr;

		if (!R.Read(Info->BufferIndex)) return nullptr;
		if (Info->BufferIndex >= Buffers.size()) return nullptr;

		U32 StructIndex;
		if (!R.Read(StructIndex)) return nullptr;
		if (StructIndex != static_cast<U32>(-1))
			Info->Struct = Structs[StructIndex];

		Info->Type = static_cast<EUSMConstType>(R.Read<U8>());

		if (!R.Read(Info->LocalOffset)) return nullptr;
		if (!R.Read(Info->ElementStride)) return nullptr;
		if (!R.Read(Info->ElementCount)) return nullptr;
		if (!R.Read(Info->Columns)) return nullptr;
		if (!R.Read(Info->Rows)) return nullptr;
		if (!R.Read(Info->Flags)) return nullptr;

		auto pBuffer = static_cast<CUSMConstantBufferParam*>(Buffers[Info->BufferIndex].Get());
		pBuffer->AddShaderTypes(ShaderTypeMask);

		Info->CalculateCachedValues();

		Const = CShaderConstantParam(Info);
	}

	if (!R.Read(Count)) return nullptr;
	std::vector<PResourceParam> Resources(Count);
	for (auto& ResourcePtr : Resources)
	{
		auto ShaderTypeMask = R.Read<U8>();
		auto Name = R.Read<CStrID>();
		auto Type = static_cast<EUSMResourceType>(R.Read<U8>());
		auto RegisterStart = R.Read<U32>();
		auto RegisterCount = R.Read<U32>();
		ResourcePtr = n_new(CUSMResourceParam(Name, ShaderTypeMask, Type, RegisterStart, RegisterCount));
	}

	if (!R.Read(Count)) return nullptr;
	std::vector<PSamplerParam> Samplers(Count);
	for (auto& SamplerPtr : Samplers)
	{
		auto ShaderTypeMask = R.Read<U8>();
		auto Name = R.Read<CStrID>();
		auto RegisterStart = R.Read<U32>();
		auto RegisterCount = R.Read<U32>();
		SamplerPtr = n_new(CUSMSamplerParam(Name, ShaderTypeMask, RegisterStart, RegisterCount));
	}

	return n_new(CShaderParamTable(std::move(Consts), std::move(Buffers), std::move(Resources), std::move(Samplers)));
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
	UPTR SizeToCopy = std::min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	const bool IsNonMappable = (Usage == D3D11_USAGE_DEFAULT || Usage == D3D11_USAGE_IMMUTABLE);

	ID3D11Buffer* pBufToMap = nullptr;
	if (IsNonMappable)
	{
		// Instead of creation may use ring buffer of precreated resources!
		D3D11_BUFFER_DESC D3DDesc;
		pBuf->GetDesc(&D3DDesc);
		D3DDesc.Usage = D3D11_USAGE_STAGING;
		D3DDesc.BindFlags = 0;
		D3DDesc.MiscFlags = 0;
		D3DDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

		if (FAILED(pD3DDevice->CreateBuffer(&D3DDesc, nullptr, &pBufToMap))) FAIL;

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

	UPTR TotalSizeX = std::max<UPTR>(Desc.Width >> MipLevel, 1);
	UPTR TotalSizeY = std::max<UPTR>(Desc.Height >> MipLevel, 1);
	UPTR TotalSizeZ = std::max<UPTR>(Desc.Depth >> MipLevel, 1);

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

	ID3D11Resource* pRsrcToMap = nullptr;
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

				ID3D11Texture1D* pTex = nullptr;
				if (FAILED(pD3DDevice->CreateTexture1D(&D3DDesc, nullptr, &pTex))) FAIL;
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

				ID3D11Texture2D* pTex = nullptr;
				if (FAILED(pD3DDevice->CreateTexture2D(&D3DDesc, nullptr, &pTex))) FAIL;
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

				ID3D11Texture3D* pTex = nullptr;
				if (FAILED(pD3DDevice->CreateTexture3D(&D3DDesc, nullptr, &pTex))) FAIL;
				pRsrcToMap = pTex;

				ImageCopyFlags |= CopyImage_3DImage;

				break;
			}
		};

		// PERF: Async, immediate reading may cause stall. Allow processing multiple read requests per call or make ReadFromResource async?
		pD3DImmContext->CopySubresourceRegion(pRsrcToMap, 0, 0, 0, 0, pTexRsrc, D3D11CalcSubresource(MipLevel, ArraySlice, Desc.MipLevels), nullptr);
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
	UPTR SizeToCopy = std::min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	const int UpdateWhole = (!Offset && SizeToCopy == BufferSize);

	if (Usage == D3D11_USAGE_DEFAULT) //???update staging here too? need perf test!
	{
		if (UpdateWhole) pD3DImmContext->UpdateSubresource(pBuf, 0, nullptr, pData, 0, 0);
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
		const int IsDynamic = (Usage == D3D11_USAGE_DYNAMIC);
#if defined(_DEBUG) && DEM_RENDER_DEBUG
		if (IsDynamic && !UpdateWhole)
			Sys::Log("Render, Warning: partial write-discard to D3D11 dynamic buffer\n");
#endif
		D3D11_MAP MapType = IsDynamic ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
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

	UPTR TotalSizeX = std::max<UPTR>(Desc.Width >> MipLevel, 1);
	UPTR TotalSizeY = std::max<UPTR>(Desc.Height >> MipLevel, 1);
	UPTR TotalSizeZ = std::max<UPTR>(Desc.Depth >> MipLevel, 1);

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
		MapType = UpdateWhole ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;
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
		pD3DImmContext->UpdateSubresource(pBuffer, 0, nullptr, pData, 0, 0);
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
			pD3DImmContext->UpdateSubresource(pBuffer, 0, nullptr, CB11.GetRAMCopy(), 0, 0);
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
			// Buffer was mapped with discard. Ensure something was written, otherwise contents are invalid.
			n_assert_dbg(CB11.IsDirty());
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

bool CD3D11GPUDriver::GetUsageAccess(UPTR InAccessFlags, bool InitDataProvided, D3D11_USAGE& OutUsage, UINT& OutCPUAccess)
{
	Data::CFlags AccessFlags(InAccessFlags);
	if (AccessFlags.IsNot(Access_CPU_Write | Access_CPU_Read))
	{
		// No CPU access needed, use GPU memory
		if (InAccessFlags == Access_GPU_Read || InAccessFlags == 0)
		{
			// Use the best read-only GPU memory
			OutUsage = InitDataProvided ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
		}
		else
		{
			// Use read-write GPU memory
			OutUsage = D3D11_USAGE_DEFAULT;
		}
	}
	else if (InAccessFlags == (Access_GPU_Read | Access_CPU_Write))
	{
		// Special case - dynamic resources, frequently updateable from CPU side
		OutUsage = D3D11_USAGE_DYNAMIC;
	}
	else if (AccessFlags.IsNot(Access_GPU_Write | Access_GPU_Read))
	{
		// No GPU access needed, system RAM resource.
		// Can't be a depth-stencil buffer or a multisampled render target.
		OutUsage = D3D11_USAGE_STAGING;
	}
	else FAIL; // Unsupported combination of access capabilities

	OutCPUAccess = 0;
	if (InAccessFlags & Access_CPU_Read) OutCPUAccess |= D3D11_CPU_ACCESS_READ;
	if (InAccessFlags & Access_CPU_Write) OutCPUAccess |= D3D11_CPU_ACCESS_WRITE;

	OK;
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
		default:
		{
			Sys::Error("CD3D11GPUDriver::GetD3DMapTypeAndFlags() > Invalid map mode\n");
			return;
		}
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
		default:
		{
			Sys::Error("CD3D11GPUDriver::GetD3DCmpFunc() > invalid function");
			return D3D11_COMPARISON_NEVER;
		}
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
		default:
		{
			Sys::Error("CD3D11GPUDriver::GetD3DStencilOp() > invalid operation");
			return D3D11_STENCIL_OP_KEEP;
		}
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
		default:
		{
			Sys::Error("CD3D11GPUDriver::GetD3DBlendArg() > invalid argument");
			return D3D11_BLEND_ZERO;
		}
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
	DEM::Sys::COSWindow* pWnd = (DEM::Sys::COSWindow*)pDispatcher;
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D11SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.Get() == pWnd)
		{
			DestroySwapChain(i);
			OK; // Only one swap chain is allowed for each window
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::OnOSWindowResized(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const auto& Ev = static_cast<const Event::OSWindowResized&>(Event);
	if (Ev.ManualResizingInProgress) FAIL;

	DEM::Sys::COSWindow* pWnd = (DEM::Sys::COSWindow*)pDispatcher;
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D11SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.Get() == pWnd)
		{
			ResizeSwapChain(i, pWnd->GetWidth(), pWnd->GetHeight());
			OK; // Only one swap chain is allowed for each window
		}
	}

	FAIL;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	DEM::Sys::COSWindow* pWnd = (DEM::Sys::COSWindow*)pDispatcher;
	for (UPTR i = 0; i < SwapChains.GetCount(); ++i)
	{
		CD3D11SwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.Get() == pWnd)
		{
			if (SC.IsFullscreen()) n_verify(SwitchToWindowed(i));
			else n_verify(SwitchToFullscreen(i));
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
