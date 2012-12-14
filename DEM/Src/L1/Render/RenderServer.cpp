#include "RenderServer.h"

#include <Events/EventManager.h>
#include <dxerr.h>

namespace Render
{
ImplementRTTI(Render::CRenderServer, Core::CRefCounted);
__ImplementSingleton(CRenderServer);

bool CRenderServer::Open()
{
	n_assert(!_IsOpen);

	pD3D = Direct3DCreate9(D3D_SDK_VERSION); //!!!in N3 opened in constructor! static Get, CanCreate etc!
	if (!pD3D) FAIL;

	if (!Display.OpenWindow()) FAIL;

	if (!CreateDevice()) FAIL;

	if (!DefaultRT.isvalid())
	{
		DefaultRT.Create();
		if (!DefaultRT->CreateDefaultRT()) FAIL;
	}

	n_assert(SUCCEEDED(D3DXCreateEffectPool(&pEffectPool)));

	//!!!load shared shader!
	//also load shaders from list

	// get shared shader vars for transforms

	// init renderers requested
	// renderers must be singletons
	// cause say if 2 shape renderers, they will not share shapes etc
	// elements -> renderer -> commands to RenderSrv
	// regular lights and models are filtered and added to model renderers per-batch

	//???load frame shader(s)? on level View created (on default camera or scene creation?)

	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CRenderServer::Close()
{
	n_assert(_IsOpen);

	DefaultRT->Destroy();
	DefaultRT = NULL;

	ReleaseDevice();

	if (Display.IsWindowOpen()) Display.CloseWindow();

	SAFE_RELEASE(pD3D);

	_IsOpen = false;
}
//---------------------------------------------------------------------

bool CRenderServer::CreateDevice()
{
	n_assert(pD3D && !pD3DDevice);

	memset(&D3DPresentParams, 0, sizeof(D3DPresentParams));

	n_assert(Display.AdapterExists(Display.Adapter));

#if DEM_D3D_USENVPERFHUD
	D3DAdapter = pD3D->GetAdapterCount() - 1;
#else
	D3DAdapter = (UINT)Display.Adapter;
#endif

	SetupBufferFormats();

	D3DCAPS9 D3DCaps;
	memset(&D3DCaps, 0, sizeof(D3DCaps));
	n_assert(SUCCEEDED(pD3D->GetDeviceCaps(D3DAdapter, DEM_D3D_DEVICETYPE, &D3DCaps)));

#if DEM_D3D_DEBUG
	DWORD BhvFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	D3DPresentParams.Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
#else
	DWORD BhvFlags = D3DCREATE_FPU_PRESERVE;
	BhvFlags |= (D3DCaps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
		D3DCREATE_HARDWARE_VERTEXPROCESSING :
		D3DCREATE_SOFTWARE_VERTEXPROCESSING;
#endif

	// D3DPRESENT_INTERVAL_ONE - as _DEFAULT, but improves VSync quality at a little cost of processing time
	D3DPresentParams.PresentationInterval = Display.VSync ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
	D3DPresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	D3DPresentParams.hDeviceWindow = Display.GetAppHwnd();
	D3DPresentParams.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	D3DPresentParams.MultiSampleType = D3DMULTISAMPLE_NONE;
	D3DPresentParams.MultiSampleQuality = 0;

	// NB: May fail if can't create requested number of backbuffers
	HRESULT hr = pD3D->CreateDevice(D3DAdapter, 
									DEM_D3D_DEVICETYPE,
									Display.GetAppHwnd(),
									BhvFlags,
									&D3DPresentParams,
									&pD3DDevice);
	if (FAILED(hr))
	{
		n_error("Failed to create Direct3D device object: %s!\n", DXGetErrorString(hr));
		FAIL;
	}

	//pD3DDevice->GetDeviceCaps(&d3d9DeviceCaps);

	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	D3DVIEWPORT9 ViewPort;
	ViewPort.Width = D3DPresentParams.BackBufferWidth;
	ViewPort.Height = D3DPresentParams.BackBufferHeight;
	ViewPort.X = 0;
	ViewPort.Y = 0;
	ViewPort.MinZ = 0.0f;
	ViewPort.MaxZ = 1.0f;
	pD3DDevice->SetViewport(&ViewPort);

	//!!!CreateQueries();

	OK;
}
//---------------------------------------------------------------------

void CRenderServer::ResetDevice()
{
	n_assert(pD3DDevice);

	EventMgr->FireEvent(CStrID("OnRenderDeviceLost"));
	
	//!!!ReleaseQueries();

	// In windowed mode the cause may be a desktop display mode switch, so find new buffer formats
	if (D3DPresentParams.Windowed) SetupBufferFormats();

	HRESULT hr = pD3DDevice->TestCooperativeLevel();
	while (hr != S_OK && hr != D3DERR_DEVICENOTRESET)
	{
		n_sleep(0.01);
		hr = pD3DDevice->TestCooperativeLevel();
	}

	n_assert(SUCCEEDED(pD3DDevice->Reset(&D3DPresentParams)));

	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	D3DVIEWPORT9 ViewPort;
	ViewPort.Width = D3DPresentParams.BackBufferWidth;
	ViewPort.Height = D3DPresentParams.BackBufferHeight;
	ViewPort.X = 0;
	ViewPort.Y = 0;
	ViewPort.MinZ = 0.0f;
	ViewPort.MaxZ = 1.0f;
	pD3DDevice->SetViewport(&ViewPort);

	EventMgr->FireEvent(CStrID("OnRenderDeviceReset"));

	//!!!CreateQueries();
}
//---------------------------------------------------------------------

void CRenderServer::ReleaseDevice()
{
	n_assert(pD3D && pD3DDevice);

	//!!!UnbindD3D9Resources();

	for (int i = 1; i < MaxRenderTargetCount; i++)
		pD3DDevice->SetRenderTarget(i, NULL);
	pD3DDevice->SetDepthStencilSurface(NULL); //???need when auto depth stencil?

	//!!!ReleaseQueries();

	pD3DDevice->Release();
	pD3DDevice = NULL;
}
//---------------------------------------------------------------------

void CRenderServer::SetupBufferFormats()
{
	n_assert(pD3D);

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

	D3DFORMAT BackBufFormat;
	if (D3DPresentParams.Windowed)
	{
		CDisplayMode DesktopMode;
		Display.GetCurrentAdapterDisplayMode((CDisplay::EAdapter)D3DAdapter, DesktopMode);
		BackBufFormat = DesktopMode.PixelFormat;
	}
	else BackBufFormat = Display.GetDisplayMode().PixelFormat;

	// Make sure the device supports a D24S8 depth buffers
	HRESULT hr = pD3D->CheckDeviceFormat(	D3DAdapter,
											DEM_D3D_DEVICETYPE,
											BackBufFormat,
											D3DUSAGE_DEPTHSTENCIL,
											D3DRTYPE_SURFACE,
											D3DFMT_D24S8);
	if (FAILED(hr))
	{
		n_error("Rendering device doesn't support D24S8 depth buffer!\n");
		return;
	}

	// Check that the depth buffer format is compatible with the backbuffer format
	hr = pD3D->CheckDepthStencilMatch(	D3DAdapter,
										DEM_D3D_DEVICETYPE,
										BackBufFormat,
										BackBufFormat,
										D3DFMT_D24S8);
	if (FAILED(hr))
	{
		n_error("Backbuffer format is not compatible with D24S8 depth buffer!\n");
		return;
	}

	D3DPresentParams.BackBufferFormat = BackBufFormat;
	D3DPresentParams.BackBufferWidth = Display.GetDisplayMode().Width;
	D3DPresentParams.BackBufferHeight = Display.GetDisplayMode().Height;
	D3DPresentParams.EnableAutoDepthStencil = TRUE; //FALSE; - N3
	D3DPresentParams.AutoDepthStencilFormat = D3DFMT_D24S8;
	D3DPresentParams.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
}
//---------------------------------------------------------------------

bool CRenderServer::BeginFrame()
{
	// Assert stream VBs, IB and VLayout aren't set

	return SUCCEEDED(pD3DDevice->BeginScene());
}
//---------------------------------------------------------------------

void CRenderServer::EndFrame()
{
	n_assert(SUCCEEDED(pD3DDevice->EndScene()));

	//???is all below necessary?
	//UnbindD3D9Resources

	//d3d9Device->SetVertexShader(NULL);
	//d3d9Device->SetPixelShader(NULL);
	//for (int i = 0; i < 8; i++)
	//{
	//	d3d9Device->SetTexture(i, NULL);
	//}
	//for (int i = 0; i < MaxNumVertexStreams; i++)
	//{
	//	d3d9Device->SetStreamSource(i, NULL, 0, 0);
	//	streamVertexBuffers[i] = 0;
	//}
	//d3d9Device->SetIndices(NULL);
	//indexBuffer = 0;
}
//---------------------------------------------------------------------

// NB: Present() should be called as late as possible after EndFrame()
// to improve parallelism between the GPU and the CPU
void CRenderServer::Present()
{
	n_assert(pD3DDevice);
	if (Display.GetAppHwnd())
	{
		HRESULT hr = pD3DDevice->Present(NULL, NULL, NULL, NULL);
		if (hr == D3DERR_DEVICELOST) ResetDevice();
		else n_assert(SUCCEEDED(hr));
	}    

	//// Sync CPU thread with GPU
	//SyncGPU();
	/*
	//{
		gpuSyncQuery[frameId % numSyncQueries]->Issue(D3DISSUE_END);                              
		frameId++;
		// wait till gpu has finsihed rendering the previous frame
		while (S_FALSE == gpuSyncQuery[frameId % numSyncQueries]->GetData(NULL, 0, D3DGETDATA_FLUSH)) ;
	//}
	*/
}
//---------------------------------------------------------------------

PVertexLayout CRenderServer::GetVertexLayout(const nArray<CVertexComponent>& Components)
{
	if (!Components.Size()) return NULL;
	CStrID Signature = CVertexLayout::BuildSignature(Components);
	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAtIndex(Idx);
	PVertexLayout Layout = n_new(CVertexLayout);
	n_assert(Layout->Create(Components));
	VertexLayouts.Add(Signature, Layout);
	return Layout;
}
//---------------------------------------------------------------------

DWORD CRenderServer::ShaderFeatureStringToMask(const nString& FeatureString)
{
	DWORD Mask = 0;

	nArray<nString> Features;
	FeatureString.Tokenize("\t |", Features);
	for (int i = 0; i < Features.Size(); ++i)
	{
		CStrID Feature = CStrID(Features[i].Get());
		int Idx = ShaderFeatures.FindIndex(Feature);
		if (Idx != INVALID_INDEX) Mask |= (1 << ShaderFeatures.ValueAtIndex(Idx));
		else
		{
			int BitIdx = ShaderFeatures.Size();
			if (BitIdx >= MaxShaderFeatureCount)
			{
				n_error("ShaderFeature: more then %d unqiue shader features requested!", MaxShaderFeatureCount);
				return 0;
			}
			ShaderFeatures.Add(Feature, BitIdx);
			Mask |= (1 << BitIdx);
		}
	}
	return Mask;
}
//---------------------------------------------------------------------

/*
void
D3D9RenderDevice::UnbindD3D9Resources()
{
    this->d3d9Device->SetVertexShader(NULL);
    this->d3d9Device->SetPixelShader(NULL);
    this->d3d9Device->SetIndices(NULL);
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        this->d3d9Device->SetTexture(i, NULL);
    }
    for (i = 0; i < MaxNumVertexStreams; i++)
    {
        this->d3d9Device->SetStreamSource(i, NULL, 0, 0);
        this->streamVertexBuffers[i] = 0;
    }
    this->indexBuffer = 0;
}

D3D9RenderDevice::EndPass()
{
	UnbindD3D9Resources();
	RenderDeviceBase::EndPass();
}
//---------------------------------------------------------------------

void
D3D9RenderDevice::DiscardQueries()
{
	for (IndexT i = 0; i < numSyncQueries; ++i)
	{
		this->gpuSyncQuery[i]->Release();  	
		this->gpuSyncQuery[i] = NULL;
	}
}

//------------------------------------------------------------------------------
void
D3D9RenderDevice::SetupQueries()
{
	// create double buffer query to avoid gpu to render more than 1 frame ahead
	IndexT i;
	for (i = 0; i < numSyncQueries; ++i)
	{
		this->d3d9Device->CreateQuery(D3DQUERYTYPE_EVENT, &this->gpuSyncQuery[i]);  	
	}
}
*/
}