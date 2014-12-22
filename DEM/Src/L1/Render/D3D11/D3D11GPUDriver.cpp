#include "D3D11GPUDriver.h"

#include <Render/D3D11/D3D11DriverFactory.h>
#include <Render/D3D11/D3D11DisplayDriver.h>
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
		pSwapChain->Release();
		pSwapChain = NULL;
	}
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::Init(DWORD AdapterNumber)
{
#if DEM_D3D_USENVPERFHUD
	AdapterNumber = D3D11DrvFactory->GetAdapterCount() - 1; // NVPerfHUD adapter //???is always the last, or read adapter info?
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
		//???message?
		return hr;
	}

	//!!!log what device was created!

	OK;

///////////////

	/*
    // Create swap chain
    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof(IDXGIFactory2), reinterpret_cast<void**>(&dxgiFactory2) );
    if ( dxgiFactory2 )
    {
        // DirectX 11.1 or later
        hr = g_pd3dDevice->QueryInterface( __uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1) );
        if (SUCCEEDED(hr))
        {
            (void) g_pImmediateContext->QueryInterface( __uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory(&sd, sizeof(sd));
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd( g_pd3dDevice, g_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1 );
        if (SUCCEEDED(hr))
        {
            hr = g_pSwapChain1->QueryInterface( __uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain) );
        }

        dxgiFactory2->Release();
    }
    else
    {
        // DirectX 11.0 systems
    }

    dxgiFactory->Release();

    if (FAILED(hr))
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

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
	Sys::COSWindow* pWnd = pWindow ? pWindow : D3D11DrvFactory->GetFocusWindow();
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

	DXGI_SWAP_CHAIN_DESC SCDesc = { 0 };
	SCDesc.BufferDesc.Width = BBWidth;
	SCDesc.BufferDesc.Height = BBHeight;
	SCDesc.BufferCount = Desc.BufferCount; //???always 1 for windowed? In full-screen mode, there is a dedicated front buffer; in windowed mode, the desktop is the front buffer. (c) Docs
	SCDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SCDesc.Windowed = TRUE; // Recommended, use SwitchToFullscreen()
	SCDesc.OutputWindow = pWnd->GetHWND();

	IDXGISwapChain* pSwapChain = NULL;
	HRESULT hr = D3D11DrvFactory->GetDXGIFactory()->CreateSwapChain(pD3DDevice, &SCDesc, &pSwapChain);
	// D3D11.1 -> IDXGIFactory2::CreateSwapChainForHwnd()

	CArray<CD3D11SwapChain>::CIterator ItSC = SwapChains.Begin();
	for (; ItSC != SwapChains.End(); ++ItSC)
		if (!ItSC->IsValid()) break;

	if (ItSC == SwapChains.End())
	{
		//if (SwapChains.GetCount() >= MaxSwapChainCount) return ERR_MAX_SWAP_CHAIN_COUNT_EXCEEDED;
		ItSC = SwapChains.Reserve(1);
	}

	ItSC->Desc = Desc;
	ItSC->TargetWindow = pWnd;
	ItSC->LastWindowRect = pWnd->GetRect();

	return SwapChains.IndexOf(ItSC);

/////////////////////////

	SCDesc.BufferDesc.Format = DXGI_FORMAT_UNKNOWN; //DXGI_FORMAT_R8G8B8A8_UNORM;
	SCDesc.BufferDesc.RefreshRate.Numerator = 60;
	SCDesc.BufferDesc.RefreshRate.Denominator = 1;
	SCDesc.SampleDesc.Count = 1;
	SCDesc.SampleDesc.Quality = 0;

/////////////////////////

	//// Make sure the device supports a depth buffer specified
	////???does support D3DFMT_UNKNOWN? if not, need to explicitly determine adapter format.
	//HRESULT hr = pD3D9->CheckDeviceFormat(	Adapter,
	//										DEM_D3D_DEVICETYPE,
	//										D3DPresentParams.BackBufferFormat,
	//										D3DUSAGE_DEPTHSTENCIL,
	//										D3DRTYPE_SURFACE,
	//										D3DPresentParams.AutoDepthStencilFormat);
	//if (FAILED(hr))
	//{
	//	Sys::Error("Rendering device doesn't support D24S8 depth buffer!\n");
	//	return ERR_CREATION_ERROR;
	//}

	//// Check that the depth buffer format is compatible with the backbuffer format
	////???does support D3DFMT_UNKNOWN? if not, need to explicitly determine adapter (and also backbuffer) format.
	//hr = pD3D9->CheckDepthStencilMatch(	Adapter,
	//									DEM_D3D_DEVICETYPE,
	//									D3DPresentParams.BackBufferFormat,
	//									D3DPresentParams.BackBufferFormat,
	//									D3DPresentParams.AutoDepthStencilFormat);
	//if (FAILED(hr))
	//{
	//	Sys::Error("Backbuffer format is not compatible with D24S8 depth buffer!\n");
	//	return ERR_CREATION_ERROR;
	//}

	//pWnd->Subscribe<CD3D11GPUDriver>(CStrID("OnToggleFullscreen"), this, &CD3D11GPUDriver::OnOSWindowToggleFullscreen, &ItSC->Sub_OnToggleFullscreen);
	//pWnd->Subscribe<CD3D11GPUDriver>(CStrID("OnClosing"), this, &CD3D11GPUDriver::OnOSWindowClosing, &ItSC->Sub_OnClosing);
	//if (Desc.Flags.Is(SwapChain_AutoAdjustSize))
	//	pWnd->Subscribe<CD3D11GPUDriver>(CStrID("OnSizeChanged"), this, &CD3D11GPUDriver::OnOSWindowSizeChanged, &ItSC->Sub_OnSizeChanged);
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::DestroySwapChain(DWORD SwapChainID)
{
	//If the swap chain is in full-screen mode, before you release it you must use SetFullscreenState
	//to switch it to windowed mode. For more information about releasing a swap chain, see the
	//"Destroying a Swap Chain" section of DXGI Overview.

	if (!SwapChainExists(SwapChainID)) FAIL;
	
	CSwapChain& SC = SwapChains[SwapChainID];
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
	if (!Width && !Height) OK;

	//!!!if W & H == curr W & H early exit OK;!!!

	CSwapChain& SC = SwapChains[SwapChainID];

	//???for child window, assert that size passed is a window size?

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	if (!GetCurrD3DPresentParams(SC, D3DPresentParams)) FAIL;

	if (SC.IsFullscreen())
	{
		CDisplayMode Mode(
			Width ? Width : D3DPresentParams.BackBufferWidth,
			Height ? Height : D3DPresentParams.BackBufferHeight,
			CD3D9DriverFactory::D3DFormatToPixelFormat(D3DPresentParams.BackBufferFormat));
		if (!SC.pTargetDisplay->SupportsDisplayMode(Mode))
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

	if (SC.pSwapChain)
	{
		SC.pSwapChain->Release();
		if (FAILED(pD3DDevice->CreateAdditionalSwapChain(&D3DPresentParams, &SC.pSwapChain))) FAIL;
	}
	else if (!Reset(D3DPresentParams)) FAIL;

	SC.Desc.BackBufferWidth = D3DPresentParams.BackBufferWidth;
	SC.Desc.BackBufferHeight = D3DPresentParams.BackBufferHeight;

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay, const CDisplayMode* pMode)
{
	if (!SwapChainExists(SwapChainID)) FAIL;
	if (pDisplay && Adapter != pDisplay->GetAdapterID()) FAIL;

	CSwapChain& SC = SwapChains[SwapChainID];

	// Only one swap chain per adapter can be fullscreen in D3D9
	// Moreover, it is always an implicit swap chain, so fail on any additional one
	if (SC.pSwapChain) FAIL;

	//???if (SC.TargetWindow->IsChild()) FAIL;

	if (!pDisplay)
	{
		Sys::Error("CD3D11GPUDriver::SwitchToFullscreen > IMPLEMENT ME for pDisplay == NULL!!!");
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

	CDisplayDriver::CMonitorInfo MonInfo;
	if (!pDisplay->GetDisplayMonitorInfo(MonInfo)) FAIL;
	SC.LastWindowRect = SC.TargetWindow->GetRect();
	SC.TargetWindow->SetRect(Data::CRect(MonInfo.Left, MonInfo.Top, pMode->Width, pMode->Height), true);

	if (!Reset(D3DPresentParams)) FAIL;

	SC.Desc.BackBufferWidth = D3DPresentParams.BackBufferWidth;
	SC.Desc.BackBufferHeight = D3DPresentParams.BackBufferHeight;

	SC.pTargetDisplay = pDisplay;
	SC.TargetWindow->Subscribe<CD3D11GPUDriver>(CStrID("OnPaint"), this, &CD3D11GPUDriver::OnOSWindowPaint, &Sub_OnPaint);

	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect)
{
	if (!SwapChainExists(SwapChainID)) FAIL;

	CSwapChain& SC = SwapChains[SwapChainID];

	// Only one swap chain per adapter can be fullscreen in D3D9.
	// Moreover, it is always an implicit swap chain, so skip transition for
	// any additional swap chain, as it always is in a windowed mode.
	// Use ResizeSwapChain() to change the swap chain size.
	if (SC.pSwapChain) OK;

	if (pWindowRect)
	{
		SC.LastWindowRect.X = pWindowRect->X;
		SC.LastWindowRect.Y = pWindowRect->Y;
		if (pWindowRect->W > 0) SC.LastWindowRect.W = pWindowRect->W;
		if (pWindowRect->H > 0) SC.LastWindowRect.H = pWindowRect->H;
	}

	D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
	FillD3DPresentParams(SC.Desc, SC.TargetWindow, D3DPresentParams);

	D3DPresentParams.Windowed = TRUE;
	D3DPresentParams.BackBufferWidth = SC.LastWindowRect.W;
	D3DPresentParams.BackBufferHeight = SC.LastWindowRect.H;
	D3DPresentParams.BackBufferFormat = D3DFMT_UNKNOWN; // Uses current desktop mode

	if (!Reset(D3DPresentParams)) FAIL;

	SC.TargetWindow->SetRect(SC.LastWindowRect);

	SC.Desc.BackBufferWidth = D3DPresentParams.BackBufferWidth;
	SC.Desc.BackBufferHeight = D3DPresentParams.BackBufferHeight;

	SC.pTargetDisplay = NULL;

	Sub_OnPaint = NULL;

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
	if (IsInsideFrame || !SwapChainExists(SwapChainID)) FAIL;

	CSwapChain& SC = SwapChains[SwapChainID];

	// For swap chain: Present will fail if called between BeginScene and EndScene pairs unless the
	// render target is not the current render target. //???so don't fail if IsInsideFrame?
	HRESULT hr = SC.pSwapChain ? SC.pSwapChain->Present(NULL, NULL, NULL, NULL, 0) : pD3DDevice->Present(NULL, NULL, NULL, NULL);
	if (FAILED(hr))
	{
		if (hr == D3DERR_DEVICELOST)
		{
			CSwapChain& ImplicitSC = SwapChains[0];

			D3DPRESENT_PARAMETERS D3DPresentParams = { 0 };
			if (!GetCurrD3DPresentParams(ImplicitSC, D3DPresentParams)) FAIL;
			if (!Reset(D3DPresentParams)) FAIL;
		}
		else if (hr == D3DERR_INVALIDCALL) FAIL;
		else // D3DERR_DRIVERINTERNALERROR, D3DERR_OUTOFVIDEOMEMORY, E_OUTOFMEMORY
		{
			// Destroy and recreate device and all swap chains
			Sys::Error("CD3D11GPUDriver::Present() > IMPLEMENT ME FAILED(hr)!!!");
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

bool CD3D11GPUDriver::OnOSWindowToggleFullscreen(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd)
		{
			if (SwapChains[i].IsFullscreen()) return SwitchToWindowed(i);
			else return SwitchToFullscreen(i);
		}
	OK;
}
//---------------------------------------------------------------------

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

bool CD3D11GPUDriver::OnOSWindowPaint(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
	{
		CSwapChain& SC = SwapChains[i];
		if (SC.TargetWindow.GetUnsafe() == pWnd)
		{
			n_assert_dbg(SC.IsFullscreen());
			if (SC.pSwapChain) SC.pSwapChain->Present(NULL, NULL, NULL, NULL, 0);
			else pD3DDevice->Present(NULL, NULL, NULL, NULL);
			OK; // Only one swap chain is allowed for each window
		}
	}
	OK;
}
//---------------------------------------------------------------------

bool CD3D11GPUDriver::OnOSWindowClosing(const Events::CEventBase& Event)
{
	Data::PParams P = ((const Events::CEvent&)Event).Params;
	Sys::COSWindow* pWnd = (Sys::COSWindow*)P->Get<PVOID>(CStrID("Window"));
	for (int i = 0; i < SwapChains.GetCount(); ++i)
		if (SwapChains[i].TargetWindow.GetUnsafe() == pWnd)
		{
			DestroySwapChain(i);
			OK; // Only one swap chain is allowed for each window
		}
	OK;
}
//---------------------------------------------------------------------

}
