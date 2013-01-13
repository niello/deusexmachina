#include "RenderServer.h"

#include <Events/EventManager.h>
#include <Data/Stream.h>
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

	FFlagSkinned = ShaderFeatureStringToMask("Skinned");
	FFlagInstanced = ShaderFeatureStringToMask("Instanced");

	if (!DefaultRT.isvalid())
	{
		DefaultRT.Create();
		if (!DefaultRT->CreateDefaultRT()) FAIL;
	}
	pCurrDSSurface = DefaultRT->GetD3DDepthStencilSurface();

	n_assert(SUCCEEDED(D3DXCreateEffectPool(&pEffectPool)));

	//!!!load shared shader!
	//also load shaders from list

	// get shared shader vars for transforms

	// init renderers requested
	// renderers must be singletons
	// cause say if 2 shape renderers, they will not share shapes etc
	// elements -> renderer -> commands to RenderSrv
	// regular lights and models are filtered and added to model renderers per-batch
	//???register renderers into dictionary CStrID -> CRenderer?
	// UI, Text, Shape, NoLight, MultiLightOnePass, OneLightMultiPass, Particle
	// particle shadows - smth like ShadowPass->ParticleRenderer->RenderShadows
	//???soft shadows, transparent shadows (for semitranslucent objects)?

	//???load frame shader(s)? on level View created (on default camera or scene creation?)

	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CRenderServer::Close()
{
	n_assert(_IsOpen);

	pCurrDSSurface = NULL;
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

	//???!!!get from Display settings?
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

	CurrDepthStencilFormat =
		D3DPresentParams.EnableAutoDepthStencil ?  D3DPresentParams.AutoDepthStencilFormat : D3DFMT_UNKNOWN;

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
		// NB: In single-threaded app, engine will stuck here until device can be reset
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
	n_assert(!IsInsideFrame);

	// Assert stream VBs, IB and VLayout aren't set

	//???where? once per frame shader change
	if (!SharedShader.isvalid())
	{
		SharedShader = ShaderMgr.GetTypedResource(CStrID("Shared"));
		n_assert(SharedShader->IsLoaded());
		hLightAmbient = SharedShader->GetVarHandleByName(CStrID("LightAmbient"));
		hEyePos = SharedShader->GetVarHandleByName(CStrID("EyePos"));
		hViewProj = SharedShader->GetVarHandleByName(CStrID("ViewProjection"));
	}

	IsInsideFrame = SUCCEEDED(pD3DDevice->BeginScene());
	return IsInsideFrame;
}
//---------------------------------------------------------------------

void CRenderServer::EndFrame()
{
	n_assert(IsInsideFrame);
	n_assert(SUCCEEDED(pD3DDevice->EndScene()));
	IsInsideFrame = false;

	//???is all below necessary? PIX requires it for debugging frame
	for (int i = 0; i < MaxVertexStreamCount; ++i)
		CurrVB[i] = NULL;
	CurrVLayout = NULL;
	CurrIB = NULL;
	//!!!UnbindD3D9Resources()
}
//---------------------------------------------------------------------

// NB: Present() should be called as late as possible after EndFrame()
// to improve parallelism between the GPU and the CPU
void CRenderServer::Present()
{
	n_assert(pD3DDevice && !IsInsideFrame);
	if (Display.GetAppHwnd())
	{
		HRESULT hr = pD3DDevice->Present(NULL, NULL, NULL, NULL);
		if (hr == D3DERR_DEVICELOST) ResetDevice();
		else n_assert(SUCCEEDED(hr));
	}    

	++FrameID;

	//// Sync CPU thread with GPU
	//// wait till gpu has finsihed rendering the previous frame
	//gpuSyncQuery[frameId % numSyncQueries]->Issue(D3DISSUE_END);                              
	//++FrameID; //???why here?
	//while (S_FALSE == gpuSyncQuery[frameId % numSyncQueries]->GetData(NULL, 0, D3DGETDATA_FLUSH)) ;
}
//---------------------------------------------------------------------

void CRenderServer::SaveScreenshot(EImageFormat ImageFormat, Data::CStream& OutStream)
{
	n_assert(pD3DDevice && !IsInsideFrame);

	IDirect3DSurface9* pCaptureSurface = NULL;
	HRESULT hr = pD3DDevice->CreateOffscreenPlainSurface(	D3DPresentParams.BackBufferWidth,
															D3DPresentParams.BackBufferHeight,
															D3DPresentParams.BackBufferFormat,
															D3DPOOL_SYSTEMMEM,
															&pCaptureSurface,
															NULL);
	n_assert(SUCCEEDED(hr) && pCaptureSurface);

	// If BackBuffer(0) surface ptr changes, need to update DefaultRT RTSurface ptr every frame
	// Capturing DefaultRT is better since we always get actual data even if don't render to swap chain
	n_assert(SUCCEEDED(pD3DDevice->GetRenderTargetData(DefaultRT->GetD3DRenderTargetSurface(), pCaptureSurface)));

	ID3DXBuffer* pBuf = NULL;    
	hr = D3DXSaveSurfaceToFileInMemory(&pBuf, ImageFormat, pCaptureSurface, NULL, NULL);
	n_assert(SUCCEEDED(hr));
	pCaptureSurface->Release();

	if (OutStream.Open(Data::SAM_WRITE, Data::SAP_SEQUENTIAL)) //???or open outside? here assert IsOpen and write access
	{
		OutStream.Write(pBuf->GetBufferPointer(), pBuf->GetBufferSize());
		OutStream.Close();
	}
	pBuf->Release();
}
//---------------------------------------------------------------------

void CRenderServer::SetRenderTarget(DWORD Index, CRenderTarget* pRT)
{
	n_assert(Index < MaxRenderTargetCount);
	if (CurrRT[Index].get_unsafe() == pRT) return;

	// Restore main RT to backbuffer and autodepthstencil (or NULL if no auto)
	if (!pRT && Index == 0) pRT = DefaultRT.get_unsafe();

	IDirect3DSurface9* pRTSurface = pRT ? pRT->GetD3DRenderTargetSurface() : NULL;
	IDirect3DSurface9* pDSSurface = pRT ? pRT->GetD3DDepthStencilSurface() : NULL;

	n_assert(SUCCEEDED(pD3DDevice->SetRenderTarget(Index, pRTSurface)));

	//!!!TEST IT! DS can be set to NULL only by main RT (index 0)
	//???mb set DS only from main RT?
	if ((pDSSurface || Index == 0) && pDSSurface != pCurrDSSurface)
	{
		CurrDepthStencilFormat = pRT ? pRT->GetDepthStencilFormat() : D3DFMT_UNKNOWN;
		pCurrDSSurface = pDSSurface;
		n_assert(SUCCEEDED(pD3DDevice->SetDepthStencilSurface(pDSSurface)));
	}

	CurrRT[Index] = pRT;
}
//---------------------------------------------------------------------

void CRenderServer::SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex)
{
	n_assert(Index < MaxVertexStreamCount && OffsetVertex < pVB->GetVertexCount());
	if (CurrVB[Index].get_unsafe() == pVB && CurrVBOffset[Index] == OffsetVertex) return;
	IDirect3DVertexBuffer9* pD3DVB = pVB ? pVB->GetD3DBuffer() : NULL;
	DWORD VertexSize = pVB ? pVB->GetVertexLayout()->GetVertexSize() : 0;
	n_assert(SUCCEEDED(pD3DDevice->SetStreamSource(Index, pD3DVB, VertexSize * OffsetVertex, VertexSize)));
	CurrVB[Index] = pVB;
	CurrVBOffset[Index] = OffsetVertex;
}
//---------------------------------------------------------------------

void CRenderServer::SetVertexLayout(CVertexLayout* pVLayout)
{
	if (CurrVLayout.get_unsafe() == pVLayout) return;
	IDirect3DVertexDeclaration9* pDecl = pVLayout ? pVLayout->GetD3DVertexDeclaration() : NULL;
	n_assert(SUCCEEDED(pD3DDevice->SetVertexDeclaration(pDecl)));
	CurrVLayout = pVLayout;
}
//---------------------------------------------------------------------

void CRenderServer::SetIndexBuffer(CIndexBuffer* pIB)
{
	if (CurrIB.get_unsafe() == pIB) return;
	IDirect3DIndexBuffer9* pD3DIB = pIB ? pIB->GetD3DBuffer() : NULL;
	n_assert(SUCCEEDED(pD3DDevice->SetIndices(pD3DIB)));
	CurrIB = pIB;
}
//---------------------------------------------------------------------

// Docs: Note that D3DSTREAMSOURCE_INDEXEDDATA and the number of instances to draw must always be set in stream zero.
void CRenderServer::SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex)
{
	n_assert2(!pVB || Instances, "CRenderServer::SetInstanceBuffer() -> Specify 1 or more instances!");
	n_assert(Index > 0); //???force instance buffer Index to be always the same (1 or smth)?

	SetVertexBuffer(Index, pVB, OffsetVertex);

	DWORD NewInstanceCount = pVB ? Instances : 0;
	if (NewInstanceCount == InstanceCount) return;
	InstanceCount = NewInstanceCount;

	if (InstanceCount > 0)
	{
		pD3DDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | InstanceCount);
		pD3DDevice->SetStreamSourceFreq(Index, D3DSTREAMSOURCE_INSTANCEDATA | 1);
	}
	else
	{
		pD3DDevice->SetStreamSourceFreq(0, 1);
		pD3DDevice->SetStreamSourceFreq(Index, 1);
	}
}
//---------------------------------------------------------------------

void CRenderServer::Clear(DWORD Flags, DWORD Color, float Depth, uchar Stencil)
{
	if (!Flags) return;

	DWORD D3DFlags = 0;

	if (Flags & Clear_Color) D3DFlags |= D3DCLEAR_TARGET;

	if (CurrDepthStencilFormat != PixelFormat_Invalid)
	{
		if (Flags & Clear_Depth) D3DFlags |= D3DCLEAR_ZBUFFER;

		if (Flags & Clear_Stencil)
		{
			bool HasStencil =
				CurrDepthStencilFormat == D3DFMT_D24S8 ||
				CurrDepthStencilFormat == D3DFMT_D24X4S4 ||
				CurrDepthStencilFormat == D3DFMT_D24FS8 ||
				CurrDepthStencilFormat == D3DFMT_D15S1;
			if (HasStencil) D3DFlags |= D3DCLEAR_STENCIL;
		}
	}

	n_assert(SUCCEEDED(pD3DDevice->Clear(0, NULL, D3DFlags, Color, Depth, Stencil)));
}
//---------------------------------------------------------------------

void CRenderServer::Draw()
{
	n_assert_dbg(pD3DDevice && IsInsideFrame);

	//???map enum to constants defined in platform-specific file like D3D9Fwd?
	D3DPRIMITIVETYPE D3DPrimType;
	DWORD PrimCount;
	switch (CurrPrimGroup.Topology)
	{
		case PointList:	D3DPrimType = D3DPT_POINTLIST; PrimCount = CurrPrimGroup.VertexCount; break;
		case LineList:	D3DPrimType = D3DPT_LINELIST; PrimCount = CurrPrimGroup.VertexCount / 2; break;
		case LineStrip:	D3DPrimType = D3DPT_LINESTRIP; PrimCount = CurrPrimGroup.VertexCount - 1; break;
		case TriList:	D3DPrimType = D3DPT_TRIANGLELIST; PrimCount = CurrPrimGroup.VertexCount / 3; break;
		case TriStrip:	D3DPrimType = D3DPT_TRIANGLESTRIP; PrimCount = CurrPrimGroup.VertexCount - 2; break;
		default:		n_error("CRenderServer::Draw() -> Invalid primitive topology!"); return;
	}

	HRESULT hr;
	if (CurrPrimGroup.IndexCount > 0)
	{
		n_assert_dbg(CurrIB.isvalid());
		n_assert_dbg(!InstanceCount || CurrVB[0].isvalid());
		hr = pD3DDevice->DrawIndexedPrimitive(	D3DPrimType,
												0,
												CurrPrimGroup.FirstVertex,
												CurrPrimGroup.VertexCount,
												CurrPrimGroup.FirstIndex,
												PrimCount);
	}
	else
	{
		n_assert2_dbg(!InstanceCount, "Non-indexed instanced rendereng is not supported by design!");
		hr = pD3DDevice->DrawPrimitive(D3DPrimType, CurrPrimGroup.FirstVertex, PrimCount);
	}
	n_assert(SUCCEEDED(hr));

	//!!!update Primitive and DIP/DP counters! if instanced, mul Primitive counter to number of instances
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

EPixelFormat CRenderServer::GetPixelFormat(const nString& String)
{
	if (String.IsEmpty()) return PixelFormat_Invalid;

	//!!!PERF!
	//!!!REWRITE in more elegant and optimal way!
	if (String == "X8R8G8B8") return D3DFMT_X8R8G8B8;
	if (String == "A8R8G8B8") return D3DFMT_A8R8G8B8;
	if (String == "R5G6B5") return D3DFMT_R5G6B5;
	if (String == "R16F") return D3DFMT_R16F;
	if (String == "G16R16F") return D3DFMT_G16R16F;
	if (String == "A16B16G16R16F") return D3DFMT_A16B16G16R16F;
	if (String == "R32F") return D3DFMT_R32F;
	if (String == "G32R32F") return D3DFMT_G32R32F;
	if (String == "A32B32G32R32F") return D3DFMT_A32B32G32R32F;
	if (String == "D32") return D3DFMT_D32;
	if (String == "D15S1") return D3DFMT_D15S1;
	if (String == "D24S8") return D3DFMT_D24S8;
	if (String == "D24X8") return D3DFMT_D24X8;
	if (String == "D24X4S4") return D3DFMT_D24X4S4;

	n_error("CRenderServer::GetPixelFormat() -> Format %s not found!\n", String.Get());
	return PixelFormat_Invalid;
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
void D3D9RenderDevice::UnbindD3D9Resources()
{
    d3d9Device->SetVertexShader(NULL);
    d3d9Device->SetPixelShader(NULL);
    for (int i = 0; i < MaxTextureStageCount; ++i)
        d3d9Device->SetTexture(i, NULL);
    for (int i = 0; i < MaxNumVertexStreams; ++i)
		SetVertexBuffer(i, NULL);
    SetIndexBuffer(NULL);
}
//---------------------------------------------------------------------
//!!!my device has no EndPass!
D3D9RenderDevice::EndPass()
{
	UnbindD3D9Resources();
	RenderDeviceBase::EndPass();
}
//---------------------------------------------------------------------
void D3D9RenderDevice::DiscardQueries()
{
	for (IndexT i = 0; i < numSyncQueries; ++i)
		SAFE_RELEASE(gpuSyncQuery[i]);  	
}
//------------------------------------------------------------------------------
void D3D9RenderDevice::SetupQueries()
{
	// create double buffer query to avoid gpu to render more than 1 frame ahead
	for (int i = 0; i < numSyncQueries; ++i)
		d3d9Device->CreateQuery(D3DQUERYTYPE_EVENT, &gpuSyncQuery[i]);  	
}
*/
}