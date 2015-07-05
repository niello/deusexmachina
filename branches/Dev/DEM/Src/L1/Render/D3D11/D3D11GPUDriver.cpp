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

bool CD3D11GPUDriver::Init(DWORD AdapterNumber, EGPUDriverType DriverType)
{
	if (!CGPUDriver::Init(AdapterNumber, DriverType)) FAIL;

	n_assert(AdapterID == Adapter_AutoSelect || D3D11DrvFactory->AdapterExists(AdapterID));

	//!!!fix if will be multithreaded, forex job-based!
	UINT CreateFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

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
	DWORD FeatureLevelCount = sizeof_array(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel; //???to member or always get from device?

	HRESULT hr = D3D11CreateDevice(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, CreateFlags,
								   FeatureLevels, FeatureLevelCount, D3D11_SDK_VERSION,
								   &pD3DDevice, &FeatureLevel, &pD3DImmContext);

	//if (hr == E_INVALIDARG)
	//{
	//	// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
	//	hr = D3D11CreateDevice(	pAdapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, CreateFlags, FeatureLevels + 1, FeatureLevelCount - 1,
	//							D3D11_SDK_VERSION, &pD3DDevice, &FeatureLevel, &pD3DImmContext);
	//}

	pAdapter->Release();

	if (FAILED(hr))
	{
		Sys::Error("Failed to create Direct3D11 device object!\n");
		FAIL;
	}

	if (AdapterID == 0) Type = GPU_Hardware; //???else? //!!!in D3D9 type was in device caps!

	Sys::Log("Device created: %s, feature level 0x%x\n", "HAL", (int)FeatureLevel);

	DWORD MRTCount = 0;
	if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0) MRTCount = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
	else if (FeatureLevel >= D3D_FEATURE_LEVEL_10_0) MRTCount = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
	else if (FeatureLevel >= D3D_FEATURE_LEVEL_9_3) MRTCount = D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT;
	else if (FeatureLevel >= D3D_FEATURE_LEVEL_9_1) MRTCount = D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT;
	CurrRT.SetSize(MRTCount);

	// To determine whether viewport is set or not at a particular index, we use a bitfield.
	// So we are not only limited by a D3D11 caps but also by a bit count in a mask.
	//???!!!use each bit for pair, is there any reason to set VP & SR separately?!
	//???and also one dirty flag? when set VP do D3D11 remember its SR or it resets SR size?
	//!!!need static assert!
	n_assert_dbg(sizeof(VPSRSetFlags) * 4 >= VP_OR_SR_SET_FLAG_COUNT);
	DWORD MaxViewportCountCaps = 1;
	if (FeatureLevel >= D3D_FEATURE_LEVEL_11_0) MaxViewportCountCaps = D3D10_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
	else if (FeatureLevel >= D3D_FEATURE_LEVEL_10_0) MaxViewportCountCaps = D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1;
	MaxViewportCount = n_min(MaxViewportCountCaps, VP_OR_SR_SET_FLAG_COUNT);
	CurrVP = n_new_array(D3D11_VIEWPORT, MaxViewportCount);
	CurrSR = n_new_array(RECT, MaxViewportCount);
	VPSRSetFlags.ClearAll();

	OK;

///////////////

	/*
    // Compile the vertex shader
    ID3DBlob* pVSBlob = nullptr;
    hr = CompileShaderFromFile( L"Tutorial07.fx", "VS", "vs_4_0", &pVSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the vertex shader
    hr = g_pd3dDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader );
    if( FAILED( hr ) )
    {    
        pVSBlob->Release();
        return hr;
    }

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    UINT numElements = ARRAYSIZE( layout );

    // Create the input layout
    hr = g_pd3dDevice->CreateInputLayout( layout, numElements, pVSBlob->GetBufferPointer(),
                                          pVSBlob->GetBufferSize(), &g_pVertexLayout );
    pVSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    // Set the input layout
    g_pImmediateContext->IASetInputLayout( g_pVertexLayout );

    // Compile the pixel shader
    ID3DBlob* pPSBlob = nullptr;
    hr = CompileShaderFromFile( L"Tutorial07.fx", "PS", "ps_4_0", &pPSBlob );
    if( FAILED( hr ) )
    {
        MessageBox( nullptr,
                    L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK );
        return hr;
    }

    // Create the pixel shader
    hr = g_pd3dDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader );
    pPSBlob->Release();
    if( FAILED( hr ) )
        return hr;

    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( SimpleVertex ) * 24;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory( &InitData, sizeof(InitData) );
    InitData.pSysMem = vertices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pVertexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set vertex buffer
    UINT stride = sizeof( SimpleVertex );
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers( 0, 1, &g_pVertexBuffer, &stride, &offset );

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof( WORD ) * 36;
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bd.CPUAccessFlags = 0;
    InitData.pSysMem = indices;
    hr = g_pd3dDevice->CreateBuffer( &bd, &InitData, &g_pIndexBuffer );
    if( FAILED( hr ) )
        return hr;

    // Set index buffer
    g_pImmediateContext->IASetIndexBuffer( g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0 );

    // Set primitive topology
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // Create the constant buffers
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(CBNeverChanges);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = 0;
    hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pCBNeverChanges );
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangeOnResize);
    hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pCBChangeOnResize );
    if( FAILED( hr ) )
        return hr;
    
    bd.ByteWidth = sizeof(CBChangesEveryFrame);
    hr = g_pd3dDevice->CreateBuffer( &bd, nullptr, &g_pCBChangesEveryFrame );
    if( FAILED( hr ) )
        return hr;

    // Load the Texture
    hr = CreateDDSTextureFromFile( g_pd3dDevice, L"seafloor.dds", nullptr, &g_pTextureRV );
    if( FAILED( hr ) )
        return hr;

    // Create the sample state
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory( &sampDesc, sizeof(sampDesc) );
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
    if( FAILED( hr ) )
        return hr;

    // Initialize the world matrices
    g_World = XMMatrixIdentity();

    // Initialize the view matrix
    XMVECTOR Eye = XMVectorSet( 0.0f, 3.0f, -6.0f, 0.0f );
    XMVECTOR At = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
    g_View = XMMatrixLookAtLH( Eye, At, Up );

    CBNeverChanges cbNeverChanges;
    cbNeverChanges.mView = XMMatrixTranspose( g_View );
    g_pImmediateContext->UpdateSubresource( g_pCBNeverChanges, 0, nullptr, &cbNeverChanges, 0, 0 );

    // Initialize the projection matrix
    g_Projection = XMMatrixPerspectiveFovLH( XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f );
    
    CBChangeOnResize cbChangesOnResize;
    cbChangesOnResize.mProjection = XMMatrixTranspose( g_Projection );
    g_pImmediateContext->UpdateSubresource( g_pCBChangeOnResize, 0, nullptr, &cbChangesOnResize, 0, 0 );
///////////////
*/
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
	ShaderInputSignatures.Clear();
	RenderStates.Clear();

	//!!!if code won't be reused in Reset(), call DestroySwapChain()!
	for (int i = 0; i < SwapChains.GetCount() ; ++i)
		if (SwapChains[i].IsValid()) SwapChains[i].Destroy();

	SAFE_DELETE_ARRAY(CurrVP);
	SAFE_DELETE_ARRAY(CurrSR);

	CurrRT.SetSize(0);

	//ctx->ClearState()
	//!!!UnbindD3DResources();
	//!!!can call the same event as on lost device!

	//for (int i = 1; i < MaxRenderTargetCount; i++)
	//	pD3DDevice->SetRenderTarget(i, NULL);
	//pD3DDevice->SetDepthStencilSurface(NULL); //???need when auto depth stencil?

	//EventSrv->FireEvent(CStrID("OnRenderDeviceRelease"));

	//!!!ReleaseQueries();

	SAFE_RELEASE(pD3DImmContext);
	SAFE_RELEASE(pD3DDevice);

//	IsInsideFrame = false;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::CheckCaps(ECaps Cap)
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

DWORD CD3D11GPUDriver::GetMaxVertexStreams()
{
	D3D_FEATURE_LEVEL FeatLevel = pD3DDevice->GetFeatureLevel();
	if (FeatLevel >= D3D_FEATURE_LEVEL_11_0) return D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	else if (FeatLevel >= D3D_FEATURE_LEVEL_10_0) return D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	else return 4; // Relatively safe value
}
//---------------------------------------------------------------------

DWORD CD3D11GPUDriver::GetMaxTextureSize(ETextureType Type)
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
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd) return ERR_CREATION_ERROR;

	UINT BBWidth = BackBufferDesc.Width, BBHeight = BackBufferDesc.Height;
	PrepareWindowAndBackBufferSize(*pWnd, BBWidth, BBHeight);

	// If VSync, use triple buffering by default, else double //!!! + 1 if front buffer must be included!
	DWORD BackBufferCount = SwapChainDesc.BackBufferCount ?
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

bool CD3D11GPUDriver::DestroySwapChain(DWORD SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	
	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	for (DWORD i = 0; i < CurrRT.GetCount(); ++i)
		if (CurrRT[i].GetUnsafe() == SC.BackBufferRT.GetUnsafe())
		{
			SetRenderTarget(i, NULL);
			break; // Some RT may be only in one slot at a time
		}

	SC.Destroy();

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwapChainExists(DWORD SwapChainID) const
{
	return SwapChainID < (DWORD)SwapChains.GetCount() && SwapChains[SwapChainID].IsValid();
}
//---------------------------------------------------------------------

//!!!call ResizeTarget to resize fullscreen or windowed to resize a target window too!
//!!!use what is written now to respond do changes!
// Does not resize an OS window, since often is called in response to an OS window resize
//???bool ResizeOSWindow?
bool CD3D11GPUDriver::ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height)
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

	DWORD RemovedRTIdx;
	for (RemovedRTIdx = 0; RemovedRTIdx < CurrRT.GetCount(); ++RemovedRTIdx)
		if (CurrRT[RemovedRTIdx] == SC.BackBufferRT)
		{
			CurrRT[RemovedRTIdx] = NULL;
			//!!!commit changes! pD3DImmContext->OMSetRenderTargets(0, NULL, NULL);
			break;
		}

	SC.BackBufferRT->Destroy();

	UINT SCFlags = 0; //DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

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

bool CD3D11GPUDriver::SwitchToFullscreen(DWORD SwapChainID, CDisplayDriver* pDisplay, const CDisplayMode* pMode)
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

bool CD3D11GPUDriver::SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect)
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

bool CD3D11GPUDriver::IsFullscreen(DWORD SwapChainID) const
{
	return SwapChainExists(SwapChainID) && SwapChains[SwapChainID].IsFullscreen();
}
//---------------------------------------------------------------------

PRenderTarget CD3D11GPUDriver::GetSwapChainRenderTarget(DWORD SwapChainID) const
{
	return SwapChainExists(SwapChainID) ? SwapChains[SwapChainID].BackBufferRT : PRenderTarget();
}
//---------------------------------------------------------------------

// NB: Present() should be called as late as possible after EndFrame()
// to improve parallelism between the GPU and the CPU
bool CD3D11GPUDriver::Present(DWORD SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	CD3D11SwapChain& SC = SwapChains[SwapChainID];
	return SUCCEEDED(SC.pSwapChain->Present(0, 0));
}
//---------------------------------------------------------------------

//!!!!!!!!!!!!???how to handle set viewport and then set another RT?

bool CD3D11GPUDriver::SetViewport(DWORD Index, const CViewport* pViewport)
{
	if (Index >= MaxViewportCount) FAIL;

	D3D11_VIEWPORT& CurrViewport = CurrVP[Index];
	DWORD IsSetBit = (1 << Index);
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

bool CD3D11GPUDriver::GetViewport(DWORD Index, CViewport& OutViewport)
{
	if (Index >= MaxViewportCount || (Index > 0 && VPSRSetFlags.IsNot(1 << Index))) FAIL;

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

bool CD3D11GPUDriver::SetScissorRect(DWORD Index, const Data::CRect* pScissorRect)
{
	if (Index >= MaxViewportCount) FAIL;

	RECT& CurrRect = CurrSR[Index];
	DWORD IsSetBit = (1 << (VP_OR_SR_SET_FLAG_COUNT + Index)); // Use higher half of bits for SR
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
		CurrRect.top = pScissorRect->X;
		CurrRect.right = pScissorRect->Right();
		CurrRect.bottom = pScissorRect->Bottom();

		VPSRSetFlags.Set(IsSetBit);
	}
	else
	{
		Sys::Error("FILL WITH RT DEFAULTS!!!\n");
		CurrRect.left = pScissorRect->X;
		CurrRect.top = pScissorRect->X;
		CurrRect.right = pScissorRect->Right();
		CurrRect.bottom = pScissorRect->Bottom();

		VPSRSetFlags.Clear(IsSetBit);
	}

	CurrDirtyFlags.Set(GPU_Dirty_SR);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::GetScissorRect(DWORD Index, Data::CRect& OutScissorRect)
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

bool CD3D11GPUDriver::BeginFrame()
{
	OK;
}
//---------------------------------------------------------------------

void CD3D11GPUDriver::EndFrame()
{
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SetRenderTarget(DWORD Index, CRenderTarget* pRT)
{
	if (Index >= CurrRT.GetCount()) FAIL;
	if (CurrRT[Index].GetUnsafe() == pRT) OK;

#ifdef _DEBUG // Can't set the same RT to more than one slot
	if (pRT)
		for (DWORD i = 0; i < CurrRT.GetCount(); ++i)
			if (CurrRT[i].GetUnsafe() == pRT) FAIL;
#endif

	CurrRT[Index] = (CD3D11RenderTarget*)pRT;
	CurrDirtyFlags.Set(GPU_Dirty_RT);

	FAIL;
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

// Even if RTs and DS set are still dirty (not bound), clear operation can be performed.
void CD3D11GPUDriver::Clear(DWORD Flags, const vector4& ColorRGBA, float Depth, uchar Stencil)
{
	if (Flags & Clear_Color)
	{
		for (DWORD i = 0; i < CurrRT.GetCount(); ++i)
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

DWORD CD3D11GPUDriver::ApplyChanges(DWORD ChangesToUpdate)
{
	if (CurrDirtyFlags.IsNotAll()) return 0;

	Data::CFlags Update(ChangesToUpdate);
	DWORD Errors = 0;

	CD3D11RenderTarget* pValidRT = NULL;

	// All render targets and a depth-stencil buffer are set atomically in D3D11
	if (Update.IsAny(GPU_Dirty_RT | GPU_Dirty_DS) && CurrDirtyFlags.IsAny(GPU_Dirty_RT | GPU_Dirty_DS))
	{
		ID3D11RenderTargetView** pRTV = (ID3D11RenderTargetView**)_malloca(sizeof(ID3D11RenderTargetView*) * CurrRT.GetCount());
		n_assert(pRTV);
		for (DWORD i = 0; i < CurrRT.GetCount(); ++i)
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
			DWORD RTWidth = pValidRT->GetDesc().Width;
			DWORD RTHeight = pValidRT->GetDesc().Height;
			bool UnsetVPFound = false;
			for (DWORD i = 0; i < MaxViewportCount; ++i)
			{
				if (VPSRSetFlags.Is(1 << i)) continue;
				
				D3D11_VIEWPORT& D3DVP = CurrVP[i];
				if (!UnsetVPFound)
				{
					// All other default values are fixed: X = 0, Y = 0, MinDepth = 0.f, MaxDepth = 1.f
					if ((DWORD)D3DVP.Width == RTWidth && (DWORD)D3DVP.Height == RTHeight) break;
					UnsetVPFound = true;
					CurrDirtyFlags.Set(GPU_Dirty_VP);
				}

				// If we are here, RT dimensions were changed, so update default VP values
				D3DVP.Width = (float)RTWidth;
				D3DVP.Height = (float)RTHeight;
			}
		}
		else CurrDirtyFlags.Clear(GPU_Dirty_VP);

		CurrDirtyFlags.Clear(GPU_Dirty_RT | GPU_Dirty_DS);
	}

	//???set viewports and scissor rects atomically?
	//???what hapens with set SRs when VPs are set?
	if (Update.Is(GPU_Dirty_VP) && CurrDirtyFlags.Is(GPU_Dirty_VP))
	{
		// Find the last set VP, as we must set all viewports from 0'th to it
		UINT NumVP = 0;
		for (DWORD i = 0; i < MaxViewportCount; ++i)
			if (VPSRSetFlags.Is(1 << i)) NumVP = i + 1;

		// At least one default viewport must be specified
		if (!NumVP) NumVP = 1;

		pD3DImmContext->RSSetViewports(NumVP, CurrVP);

		CurrDirtyFlags.Clear(GPU_Dirty_VP);
	}

	return Errors;
}
//---------------------------------------------------------------------

//???is AddRef invoked if runtime finds existing state?
//???allow relative states? or always absolute and filter out redundant Set() calls?
PRenderState CD3D11GPUDriver::CreateRenderState(const Data::CParams& Desc)
{
	D3D11_RASTERIZER_DESC RDesc;
	RDesc.DepthClipEnable = Desc.Get(CStrID("DepthClip"), true);
	// Init desc, use default values when unspecified
	ID3D11RasterizerState* pRState = NULL;
	if (FAILED(pD3DDevice->CreateRasterizerState(&RDesc, &pRState))) return NULL;

	D3D11_DEPTH_STENCIL_DESC DSDesc;
	DSDesc.DepthEnable = Desc.Get(CStrID("DepthEnable"), true);
	// Init desc, use default values when unspecified
	ID3D11DepthStencilState* pDSState = NULL;
	if (FAILED(pD3DDevice->CreateDepthStencilState(&DSDesc, &pDSState)))
	{
		pRState->Release();
		return NULL;
	}

	D3D11_BLEND_DESC BDesc;
	BDesc.IndependentBlendEnable = Desc.Get(CStrID("IndependentBlendPerTarget"), false);
	BDesc.AlphaToCoverageEnable = Desc.Get(CStrID("AlphaToCoverage"), false);
	if (BDesc.IndependentBlendEnable)
	{
		// CDataArray of sub-descs
		// Init desc, use default values when unspecified
		//!!!AVOID DUPLICATE CODE!
	}
	else
	{
		BDesc.RenderTarget[0].BlendEnable = Desc.Get(CStrID("BlendEnable"), false);
		// Init desc, use default values when unspecified
		//!!!AVOID DUPLICATE CODE!
	}
	ID3D11BlendState* pBState = NULL;
	if (FAILED(pD3DDevice->CreateBlendState(&BDesc, &pBState)))
	{
		pRState->Release();
		pDSState->Release();
		return NULL;
	}

	// Since render state creation should be load-time, it is not performance critical
	for (int i = 0; i < RenderStates.GetCount(); ++i)
	{
		CD3D11RenderState* pRS = RenderStates[i].GetUnsafe();
		if (pRS->pRState == pRState && pRS->pDSState == pDSState && pRS->pBState == pBState) return pRS;
	}

	PD3D11RenderState RS = n_new(CD3D11RenderState);
	RS->pRState = pRState;
	RS->pDSState = pDSState;
	RS->pBState = pBState;

	RenderStates.Add(RS);

	return RS.GetUnsafe();
}
//---------------------------------------------------------------------

// Gets or creates an actual layout for the given vertex layout and shader input signature
ID3D11InputLayout* CD3D11GPUDriver::GetD3DInputLayout(CD3D11VertexLayout& VertexLayout, CStrID ShaderInputSignatureID, const Data::CBuffer* pSignature)
{
	ID3D11InputLayout* pLayout = VertexLayout.GetD3DInputLayout(ShaderInputSignatureID);
	if (pLayout) return pLayout;

	const D3D11_INPUT_ELEMENT_DESC* pD3DDesc = VertexLayout.GetCachedD3DLayoutDesc();
	if (!pD3DDesc) return NULL;

	if (!pSignature)
	{
		int Idx = ShaderInputSignatures.FindIndex(ShaderInputSignatureID);
		if (Idx == INVALID_INDEX) FAIL;
		pSignature = &ShaderInputSignatures.ValueAt(Idx);
	}

	if (FAILED(pD3DDevice->CreateInputLayout(pD3DDesc, VertexLayout.GetComponentCount(), pSignature->GetPtr(), pSignature->GetSize(), &pLayout))) return NULL;

	n_verify_dbg(VertexLayout.AddLayoutObject(ShaderInputSignatureID, pLayout));

	return pLayout;
}
//---------------------------------------------------------------------

// Creates transparent user object
PVertexLayout CD3D11GPUDriver::CreateVertexLayout(const CVertexComponent* pComponents, DWORD Count)
{
	const DWORD MAX_VERTEX_COMPONENTS = 32;

	if (!pComponents || !Count || Count > MAX_VERTEX_COMPONENTS) return NULL;

	CStrID Signature = CVertexLayout::BuildSignature(pComponents, Count);

	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx).GetUnsafe();

	DWORD MaxVertexStreams = GetMaxVertexStreams();

	D3D11_INPUT_ELEMENT_DESC DeclData[MAX_VERTEX_COMPONENTS] = { 0 };
	for (DWORD i = 0; i < Count; i++)
	{
		const CVertexComponent& Component = pComponents[i];

		DWORD StreamIndex = Component.Stream;
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

PVertexBuffer CD3D11GPUDriver::CreateVertexBuffer(CVertexLayout& VertexLayout, DWORD VertexCount, DWORD AccessFlags, const void* pData)
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

PIndexBuffer CD3D11GPUDriver::CreateIndexBuffer(EIndexType IndexType, DWORD IndexCount, DWORD AccessFlags, const void* pData)
{
	if (!pD3DDevice || !IndexCount) return NULL;

	D3D11_USAGE Usage; // GetUsageAccess() never returns immutable usage if data is not provided
	UINT CPUAccess;
	GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess);

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = Usage;
	Desc.ByteWidth = IndexCount * (DWORD)IndexType;
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

// pData - initial data to be uploaded to a texture.
// if MipDataProvided, order is ArrayElement[0] { Mip[0] ... Mip[N] } ... ArrayElement[M] { Mip[0] ... Mip[N] },
// else order is ArrayElement[0] { Mip[0] } ... ArrayElement[M] { Mip[0] }, where Mip[0] is an original data.
PTexture CD3D11GPUDriver::CreateTexture(const CTextureDesc& Desc, DWORD AccessFlags, const void* pData, bool MipDataProvided)
{
	if (!pD3DDevice || !Desc.Width || !Desc.Height) return NULL;

	PD3D11Texture Tex = n_new(CD3D11Texture);
	if (Tex.IsNullPtr()) return NULL;

	DXGI_FORMAT DXGIFormat = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);
	UINT QualityLvlCount = 0;
	if (Desc.MSAAQuality != MSAA_None)
	{
		if (FAILED(pD3DDevice->CheckMultisampleQualityLevels(DXGIFormat, (int)Desc.MSAAQuality, &QualityLvlCount)) || !QualityLvlCount) return NULL;
		pData = NULL; // MSAA resources can't be initialized with data
	}

	D3D11_USAGE Usage; // GetUsageAccess() never returns immutable usage if data is not provided
	UINT CPUAccess;
	GetUsageAccess(AccessFlags, !!pData, Usage, CPUAccess);

	DWORD MipLevels = Desc.MipLevels;
	DWORD ArraySize = (Desc.Type == Texture_3D) ? 1 : ((Desc.Type == Texture_Cube) ? 6 * Desc.ArraySize : Desc.ArraySize);
	UINT MiscFlags = 0; //???if (MipLevels != 1) D3D11_RESOURCE_MISC_RESOURCE_CLAMP for ID3D11DeviceContext::SetResourceMinLOD()
	UINT BindFlags = (Usage != D3D11_USAGE_STAGING) ? D3D11_BIND_SHADER_RESOURCE : 0;

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
		DWORD BlockSize = CD3D11DriverFactory::DXGIFormatBlockSize(DXGIFormat);

		if (!MipLevels) MipLevels = GetMipLevelCount(Desc.Width, Desc.Height, BlockSize);

		DWORD BPP = CD3D11DriverFactory::DXGIFormatBitsPerPixel(DXGIFormat);
		n_assert_dbg(BPP > 0);

		// To avoid dynamic allocation of pitch arrays
		const DWORD MAX_MIPS = 16;
		n_assert_dbg(MipLevels <= MAX_MIPS);
		if (MipLevels > MAX_MIPS) MipLevels = MAX_MIPS;

		//???texture and/or utility [inline] methods GetRowPitch(level, w, [h], fmt), GetSlicePitch(level, w, h, fmt OR rowpitch, h)?
		UINT Pitch[MAX_MIPS], SlicePitch[MAX_MIPS];
		if (BlockSize == 1)
		{
			for (DWORD Mip = 0; Mip < MipLevels; ++Mip)
			{
				Pitch[Mip] = ((Desc.Width >> Mip) * BPP) >> 3;
				SlicePitch[Mip] = Pitch[Mip] * (Desc.Height >> Mip);
			}
		}
		else
		{
			for (DWORD Mip = 0; Mip < MipLevels; ++Mip)
			{
				DWORD BlockCountW = ((Desc.Width >> Mip) + BlockSize - 1) / BlockSize;
				DWORD BlockCountH = ((Desc.Height >> Mip) + BlockSize - 1) / BlockSize;
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
		char* pCurrData = (char*)pData;
		for (DWORD Elm = 0; Elm < ArraySize; ++Elm, ++pCurrInitData)
		{
			pCurrInitData->pSysMem = pCurrData;
			pCurrInitData->SysMemPitch = Pitch[0];
			pCurrInitData->SysMemSlicePitch = SlicePitch[0];
			pCurrData += SlicePitch[0];

			for (DWORD Mip = 1; Mip < MipLevels; ++Mip, ++pCurrInitData)
			{
				if (MipDataProvided)
				{
					pCurrInitData->pSysMem = pCurrData;
					pCurrInitData->SysMemPitch = Pitch[Mip];
					pCurrInitData->SysMemSlicePitch = SlicePitch[Mip];
					pCurrData += SlicePitch[Mip];
				}
				else if (pGeneratedMips)
				{
					pCurrInitData->pSysMem = pGeneratedMips;
					pCurrInitData->SysMemPitch = Pitch[Mip];
					pCurrInitData->SysMemSlicePitch = SlicePitch[Mip];
					pGeneratedMips += SlicePitch[Mip];
				}
				else
				{
					pCurrInitData->pSysMem = NULL;
					pCurrInitData->SysMemPitch = 0;
					pCurrInitData->SysMemSlicePitch = 0;
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
	else
	{
		Sys::Error("CD3D11GPUDriver::CreateTexture() > Unknown texture type %d\n", Desc.Type);
		return NULL;
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

//???allow arrays? allow 3D and cubes? will need RT.Create or CreateRenderTarget(Texture, SurfaceLocation)
PRenderTarget CD3D11GPUDriver::CreateRenderTarget(const CRenderTargetDesc& Desc)
{
	DXGI_FORMAT Fmt = CD3D11DriverFactory::PixelFormatToDXGIFormat(Desc.Format);

	UINT QualityLvlCount = 0;
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
	UINT QualityLvlCount = 0;
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
	UINT MapFlags;
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
	UINT MapFlags;
	GetD3DMapTypeAndFlags(Mode, MapType, MapFlags);

	D3D11_MAPPED_SUBRESOURCE D3DData;
	if (FAILED(pD3DImmContext->Map(pIB, 0, MapType, MapFlags, &D3DData))) FAIL;
	*ppOutData = D3DData.pData;

	OK;
}
//---------------------------------------------------------------------

// Pointer will be 16-byte aligned
bool CD3D11GPUDriver::MapResource(CMappedTexture& OutData, const CTexture& Resource, EResourceMapMode Mode, DWORD ArraySlice, DWORD MipLevel)
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
		DWORD ArrayIndex = ArraySlice / 6;
		DWORD Face = ArraySlice - (ArrayIndex * 6);
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
	UINT MapFlags;
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

bool CD3D11GPUDriver::UnmapResource(const CTexture& Resource, DWORD ArraySlice, DWORD MipLevel)
{
	n_assert_dbg(Resource.IsA<CD3D11Texture>());
	ID3D11Resource* pD3DTexRsrc = ((const CD3D11Texture&)Resource).GetD3DResource();
	if (!pD3DTexRsrc) FAIL;

	const CTextureDesc& TexDesc = Resource.GetDesc();

/*
	// Perform cubemap face mapping, if necesary. Can avoid it if cubemap faces are loaded in ECubeMapFace order.
	if (TexDesc.Type == Texture_Cube)
	{
		DWORD ArrayIndex = ArraySlice / 6;
		DWORD Face = ArraySlice - (ArrayIndex * 6);
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

bool CD3D11GPUDriver::WriteToD3DBuffer(ID3D11Buffer* pBuf, D3D11_USAGE Usage, DWORD BufferSize, const void* pData, DWORD Size, DWORD Offset)
{
	if (!pBuf || Usage == D3D11_USAGE_IMMUTABLE || !pData || !BufferSize) FAIL;

	DWORD RequestedSize = Size ? Size : BufferSize;
	DWORD SizeToCopy = n_min(RequestedSize, BufferSize - Offset);
	if (!SizeToCopy) OK;

	const int UpdateWhole = (!Offset && SizeToCopy == BufferSize);

	if (Usage == D3D11_USAGE_DEFAULT) //???update staging here too?
	{
		//!!!for textures - only non-DS and non-MSAA!

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

		OK;
	}

	//!!!D3DData.pData is 16-byte aligned for feature level 10 and above! may exploit it!
	//if (IsAligned16(pData)) perform SSE copy!
	D3D11_MAP MapType = (Usage == D3D11_USAGE_DYNAMIC && UpdateWhole) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
	D3D11_MAPPED_SUBRESOURCE D3DData;
	if (FAILED(pD3DImmContext->Map(pBuf, 0, MapType, 0, &D3DData))) FAIL;
	const bool Success = (memcpy_s(((char*)D3DData.pData) + Offset, SizeToCopy, pData, SizeToCopy) == 0);
	pD3DImmContext->Unmap(pBuf, 0);

	return Success;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::WriteToResource(const CVertexBuffer& Resource, const void* pData, DWORD Size, DWORD Offset)
{
	n_assert_dbg(Resource.IsA<CD3D11VertexBuffer>());
	const CD3D11VertexBuffer& VB11 = (const CD3D11VertexBuffer&)Resource;
	return WriteToD3DBuffer(VB11.GetD3DBuffer(), VB11.GetD3DUsage(), VB11.GetSizeInBytes(), pData, Size, Offset);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::WriteToResource(const CIndexBuffer& Resource, const void* pData, DWORD Size, DWORD Offset)
{
	n_assert_dbg(Resource.IsA<CD3D11IndexBuffer>());
	const CD3D11IndexBuffer& IB11 = (const CD3D11IndexBuffer&)Resource;
	return WriteToD3DBuffer(IB11.GetD3DBuffer(), IB11.GetD3DUsage(), IB11.GetSizeInBytes(), pData, Size, Offset);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::WriteToResource(const CTexture& Resource, const CMappedTexture& SrcData, DWORD ArraySlice, DWORD MipLevel, const Data::CBox* pRegion)
{
	FAIL;
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

void CD3D11GPUDriver::GetUsageAccess(DWORD InAccessFlags, bool InitDataProvided, D3D11_USAGE& OutUsage, UINT& OutCPUAccess)
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

void CD3D11GPUDriver::GetD3DMapTypeAndFlags(EResourceMapMode MapMode, D3D11_MAP& OutMapType, UINT& OutMapFlags)
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

bool CD3D11GPUDriver::OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	Sys::COSWindow* pWnd = (Sys::COSWindow*)pDispatcher;
	for (int i = 0; i < SwapChains.GetCount(); ++i)
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
	for (int i = 0; i < SwapChains.GetCount(); ++i)
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
	for (int i = 0; i < SwapChains.GetCount(); ++i)
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
