#include "D3D11GPUDriver.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11DisplayDriver.h>
#include <Render/D3D11/D3D11RenderState.h>
#include <Events/EventServer.h>
#include <System/OSWindow.h>
#include <Core/Factory.h>

#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>

namespace Render
{
__ImplementClass(Render::CD3D11GPUDriver, '11GD', Render::CGPUDriver);

//???to level-1 class?
void CD3D11GPUDriver::CD3D11SwapChain::Release()
{
	Sub_OnClosing = NULL;
	Sub_OnSizeChanged = NULL;
	Sub_OnToggleFullscreen = NULL;

	if (pSwapChain)
	{
		if (IsFullscreen()) pSwapChain->SetFullscreenState(FALSE, NULL);
		pSwapChain->Release();
		pSwapChain = NULL;
	}
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::Init(DWORD AdapterNumber)
{
#if DEM_D3D_USENVPERFHUD
	Sys::Error("IMPLEMENT ME!!! NVPerfHUD.");
//UINT nAdapter = 0;
//IDXGIAdapter* adapter = NULL;
//IDXGIAdapter* selectedAdapter = NULL;
//D3D10_DRIVER_TYPE driverType = D3D10_DRIVER_TYPE_HARDWARE;  
//while (pDXGIFactory->EnumAdapters(nAdapter, &adapter) != DXGI_ERROR_NOT_FOUND)
//{
//	if (adapter)
//	{
//		DXGI_ADAPTER_DESC adaptDesc;
//		if (SUCCEEDED(adapter->GetDesc(&adaptDesc)))
//		{
//			const bool isPerfHUD = wcscmp(adaptDesc.Description, L"NVIDIA PerfHUD") == 0;  
//			// Select the first adapter in normal circumstances or the PerfHUD one if it exists.  
//			if (nAdapter == 0 || isPerfHUD) selectedAdapter = adapter;  
//			if (isPerfHUD) driverType = D3D10_DRIVER_TYPE_REFERENCE;  
//		}
//	}
//	++nAdapter;  
//}
#endif

	IDXGIAdapter1* pAdapter = NULL;
	n_assert(D3D11DrvFactory->AdapterExists(AdapterNumber));
	if (!SUCCEEDED(D3D11DrvFactory->GetDXGIFactory()->EnumAdapters1(AdapterNumber, &pAdapter))) FAIL;

	//!!!fix if will be multithreaded, forex job-based!
	UINT CreateFlags = D3D11_CREATE_DEVICE_SINGLETHREADED;

#if DEM_D3D_DEBUG
	CreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
	// D3D11.1
	//D3D11_CREATE_DEVICE_DEBUGGABLE - shader debugging
	//Shader debugging requires a driver that is implemented to the WDDM for Windows 8 (WDDM 1.2)
#endif

	//D3D_DRIVER_TYPE DriverTypes[] =
	//{
	//	D3D_DRIVER_TYPE_HARDWARE,
	//	D3D_DRIVER_TYPE_WARP
	//};
	//DWORD DriverTypeCount = sizeof_array(DriverTypes);

	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		//D3D_FEATURE_LEVEL_11_1, //!!!???use DXGI factory 2?!
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};
	DWORD FeatureLevelCount = sizeof_array(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel; //???to member or always get from device?

	HRESULT hr = E_FAIL;

	//!!!can implement this loop externally, iterating adapters, if application needs a fallback driver!
	//for (DWORD i = 0; i < DriverTypeCount; ++i)
	//{
		hr = D3D11CreateDevice(	pAdapter, D3D_DRIVER_TYPE_UNKNOWN/*DriverTypes[i]*/, NULL, CreateFlags, FeatureLevels, FeatureLevelCount,
								D3D11_SDK_VERSION, &pD3DDevice, &FeatureLevel, &pD3DImmContext);

		//if (hr == E_INVALIDARG)
		//{
		//	// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
		//	hr = D3D11CreateDevice(	pAdapter, DriverTypes[i], NULL, CreateFlags, FeatureLevels + 1, FeatureLevelCount - 1,
		//							D3D11_SDK_VERSION, &pD3DDevice, &FeatureLevel, &pD3DImmContext);
		//}

	//	if (SUCCEEDED(hr)) break;
	//}

	pAdapter->Release();

	if (FAILED(hr))
	{
		Sys::Error("Failed to create Direct3D11 device object!\n");
		FAIL;
	}

	//!!!log what device was created!

	OK;

///////////////

	/*

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

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

void CD3D11GPUDriver::Release()
{
	if (!pD3DDevice) return;

	RenderStates.Clear();

	//!!!if code won't be reused in Reset(), call DestroySwapChain()!
	for (int i = 0; i < SwapChains.GetCount() ; ++i)
		SwapChains[i].Release();

	//!!!UnbindD3D9Resources();
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
	FAIL;
}
//---------------------------------------------------------------------

DWORD CD3D11GPUDriver::CreateSwapChain(const CSwapChainDesc& Desc, Sys::COSWindow* pWindow)
{
	CArray<CD3D11SwapChain>::CIterator ItSC = SwapChains.Begin();
	for (; ItSC != SwapChains.End(); ++ItSC)
		if (!ItSC->IsValid()) break;

	//if (ItSC == SwapChains.End() && SwapChains.GetCount() >= MaxSwapChainCount) return ERR_MAX_SWAP_CHAIN_COUNT_EXCEEDED;

	Sys::COSWindow* pWnd = pWindow; //???the same as for D3D9? - pWindow ? pWindow : D3D11DrvFactory->GetFocusWindow();
	n_assert(pWnd);

	//???check all the swap chains not to use this window?

	//!!!DUPLICATE CODE! D3D9!
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

	//!!!if VSync, use triple buffering, else double!
	//!!!no VSync when windowed can cause jerking at low frame rates!

	DXGI_SWAP_CHAIN_DESC SCDesc = { 0 };

	switch (Desc.SwapMode)
	{
		case SwapMode_CopyPersist:	SCDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL; break;
		//case SwapMode_FlipPersist:	// DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL, starting from Win8, min 2 backbuffers
		case SwapMode_CopyDiscard:
		default:					SCDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; break; // Allows runtime to select the best
	}

	SCDesc.BufferDesc.Width = BBWidth;
	SCDesc.BufferDesc.Height = BBHeight;
	SCDesc.BufferDesc.Format = DXGI_FORMAT_UNKNOWN; //!!!just to try default for windowed! GetDesktopFormat()?! //DXGI_FORMAT_R8G8B8A8_UNORM; //???use SRGB?
	SCDesc.BufferDesc.RefreshRate.Numerator = 0;
	SCDesc.BufferDesc.RefreshRate.Denominator = 0;
	SCDesc.BufferCount = Desc.BackBufferCount; //!!! + 1 if front buffer must be included!
	SCDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SCDesc.Windowed = TRUE; // Recommended, use SwitchToFullscreen()
	SCDesc.OutputWindow = pWnd->GetHWND();

	// DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH - to allow automatic display mode switch on wnd->fullscr
	// else use ResizeTarget when fullscreen

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

	IDXGISwapChain* pSwapChain = NULL;
	HRESULT hr = D3D11DrvFactory->GetDXGIFactory()->CreateSwapChain(pD3DDevice, &SCDesc, &pSwapChain);

	// They say if the first failure was due to wrong BufferCount, DX sets it to the correct value
	if (FAILED(hr)) hr = D3D11DrvFactory->GetDXGIFactory()->CreateSwapChain(pD3DDevice, &SCDesc, &pSwapChain);

	if (FAILED(hr))
	{
		Sys::Error("D3D11 swap chain creation error\n");
		return ERR_CREATION_ERROR;
	}

/*
    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;
*/

	//!!!depthstencil surface! if requested!

	//If you previously called IDXGIFactory::MakeWindowAssociation, the user can press the Alt-Enter
	//key combination and DXGI will transition your application between windowed and full-screen mode.
	//IDXGIFactory::MakeWindowAssociation is recommended, because a standard control mechanism for the user is strongly desired.

	if (ItSC == SwapChains.End()) ItSC = SwapChains.Reserve(1);

	ItSC->Desc = Desc;
	ItSC->pSwapChain = pSwapChain;
	ItSC->TargetWindow = pWnd;
	ItSC->LastWindowRect = pWnd->GetRect();

	return SwapChains.IndexOf(ItSC);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::DestroySwapChain(DWORD SwapChainID)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	
	CD3D11SwapChain& SC = SwapChains[SwapChainID];
	SC.Release();
	SC.pTargetDisplay = NULL;
	SC.TargetWindow = NULL;

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwapChainExists(DWORD SwapChainID) const
{
	return SwapChainID < (DWORD)SwapChains.GetCount() && SwapChains[SwapChainID].IsValid();
}
//---------------------------------------------------------------------

// Does not resize an OS window, since often is called in response to an OS window resize
//???bool ResizeOSWindow?
bool CD3D11GPUDriver::ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	if (!Width) Width = SC.Desc.BackBufferWidth;
	if (!Height) Height = SC.Desc.BackBufferHeight;
	if (SC.Desc.BackBufferWidth == Width && SC.Desc.BackBufferHeight == Height) OK;

	//???for child window, assert that size passed is a window size?

	//???or this method must resize target? in fact, need two swap chain resizing methods?
	//one as command (ResizeTarget), one as handler (ResizeBuffers), second can be OnOSWindowSizeChanged handler

	//release all references to back buffers
	//can use device's ClearState, which must call ID3D11DeviceContext::ClearState inside for all contexts,
	//or clear only contexts that use this swap chain
	//SC.pSwapChain->ResizeBuffers(0, Width, Height, DXGI_FORMAT_UNKNOWN, 0); //!!!swap chain flags as at creation!

	SC.Desc.BackBufferWidth = Width;
	SC.Desc.BackBufferHeight = Height;

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay, const CDisplayMode* pMode)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	if (pDisplay && Adapter != pDisplay->GetAdapterID()) FAIL;

	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CD3D11SwapChain& SC = SwapChains[SwapChainID];

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::IsFullscreen(DWORD SwapChainID) const
{
	return SwapChainExists(SwapChainID) && SwapChains[SwapChainID].IsFullscreen();
}
//---------------------------------------------------------------------

// NB: Present() should be called as late as possible after EndFrame()
// to improve parallelism between the GPU and the CPU
bool CD3D11GPUDriver::Present(DWORD SwapChainID)
{
	OK;
}
//---------------------------------------------------------------------

PVertexLayout CD3D11GPUDriver::InternalCreateVertexLayout()
{
	//ID3D11InputLayout* pLayout = NULL;
	return NULL;
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

//???handle in a swap chain? if all references ae inside. If device context may have one, pass through CD3D11GPUDriver
/*
bool CD3D11GPUDriver::OnOSWindowSizeChanged(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd)
		{
			// May not fire in fullscreen mode by design, this assert checks it
			// If assertion failed, we should rewrite this handler and maybe some other code
			n_assert_dbg(!SwapChains[i].IsFullscreen());

			ResizeSwapChain(i, pWnd->GetWidth(), pWnd->GetHeight());
			OK; // Only one swap chain is allowed for each window
		}
	OK;
}
//---------------------------------------------------------------------
*/

/*
case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint( hWnd, &ps );
			EndPaint( hWnd, &ps );
			return 0;
		}

case WM_SIZE:
        if (g_pSwapChain)
        {
            g_pd3dDeviceContext->OMSetRenderTargets(0, 0, 0);

            // Release all outstanding references to the swap chain's buffers.
            g_pRenderTargetView->Release();

            HRESULT hr;
            // Preserve the existing buffer count and format.
            // Automatically choose the width and height to match the client rect for HWNDs.
            hr = g_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			//???what about fullscreen? gets target size?
			//???how to go fullscreen? what is the order of SetFullscreenState() and ResizeTarget()?
                                            
            // Perform error handling here!

            // Get buffer and create a render-target-view.
            ID3D11Texture2D* pBuffer;
            hr = g_pSwapChain->GetBuffer(0, __uuidof( ID3D11Texture2D),
                                         (void**) &pBuffer );
            // Perform error handling here!

            hr = g_pd3dDevice->CreateRenderTargetView(pBuffer, NULL,
                                                     &g_pRenderTargetView);
            // Perform error handling here!
            pBuffer->Release();

            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL );

            // Set up the viewport.
            D3D11_VIEWPORT vp;
            vp.Width = width;
            vp.Height = height;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            vp.TopLeftX = 0;
            vp.TopLeftY = 0;
            g_pd3dDeviceContext->RSSetViewports( 1, &vp );
        }
        return 1;
*/

}
