#include "RenderServer.h"

#include <Events/EventManager.h>
#include <IO/Stream.h>
#include <Core/CoreServer.h>
#include <dxerr.h>

namespace Render
{
__ImplementClassNoFactory(Render::CRenderServer, Core::CRefCounted);
__ImplementSingleton(CRenderServer);

bool CRenderServer::Open()
{
	n_assert(!_IsOpen);

	pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D) FAIL;

	Display.SetDisplayMode(Display.GetRequestedDisplayMode());
	if (!Display.OpenWindow()) FAIL;

	CDisplayMode OldMode = Display.GetDisplayMode();
	if (!CreateDevice()) FAIL;
	if (Display.GetDisplayMode() != OldMode) Display.ResetWindow();

	FFlagSkinned = ShaderFeatures.GetMask("Skinned");
	FFlagInstanced = ShaderFeatures.GetMask("Instanced");

	if (!DefaultRT.IsValid())
	{
		DefaultRT = n_new(CRenderTarget);
		if (!DefaultRT->CreateDefaultRT()) FAIL;
	}
	pCurrDSSurface = DefaultRT->GetD3DDepthStencilSurface();

	n_assert(SUCCEEDED(D3DXCreateEffectPool(&pEffectPool)));

	SUBSCRIBE_PEVENT(OnDisplayPaint, CRenderServer, OnDisplayPaint);
	SUBSCRIBE_PEVENT(OnDisplayToggleFullscreen, CRenderServer, OnToggleFullscreenWindowed);
	SUBSCRIBE_PEVENT(OnDisplaySizeChanged, CRenderServer, OnDisplaySizeChanged);

	_IsOpen = true;
	OK;
}
//---------------------------------------------------------------------

void CRenderServer::Close()
{
	n_assert(_IsOpen);

	UNSUBSCRIBE_EVENT(OnDisplayPaint);
	UNSUBSCRIBE_EVENT(OnDisplayToggleFullscreen);
	UNSUBSCRIBE_EVENT(OnDisplaySizeChanged);

	SAFE_RELEASE(pCurrDSSurface);
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

	SetupPresentParams();

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

	SetupDevice();

	OK;
}
//---------------------------------------------------------------------

void CRenderServer::ResetDevice()
{
	n_assert(pD3DDevice);

	EventMgr->FireEvent(CStrID("OnRenderDeviceLost"));

	//!!!ReleaseQueries();
	SAFE_RELEASE(pCurrDSSurface);

	HRESULT hr = pD3DDevice->TestCooperativeLevel();
	while (hr != S_OK && hr != D3DERR_DEVICENOTRESET)
	{
		// NB: In single-threaded app, engine will stuck here until device can be reset
		n_sleep(0.01);
		Display.ProcessWindowMessages();
		hr = pD3DDevice->TestCooperativeLevel();
	}

	SetupPresentParams();

	hr = pD3DDevice->Reset(&D3DPresentParams);
	if (FAILED(hr))
		n_error("Failed to reset Direct3D device object: %s!\n", DXGetErrorString(hr));

	SetupDevice();

	EventMgr->FireEvent(CStrID("OnRenderDeviceReset"));

	pCurrDSSurface = DefaultRT->GetD3DDepthStencilSurface();
}
//---------------------------------------------------------------------

void CRenderServer::ReleaseDevice()
{
	n_assert(pD3D && pD3DDevice);

	//!!!UnbindD3D9Resources();

	for (int i = 1; i < MaxRenderTargetCount; i++)
		pD3DDevice->SetRenderTarget(i, NULL);
	pD3DDevice->SetDepthStencilSurface(NULL); //???need when auto depth stencil?

	EventMgr->FireEvent(CStrID("OnRenderDeviceRelease"));

	//!!!ReleaseQueries();

	pD3DDevice->Release();
	pD3DDevice = NULL;
}
//---------------------------------------------------------------------

void CRenderServer::SetupDevice()
{
	pD3DDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	pD3DDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

	D3DVIEWPORT9 ViewPort;
	ViewPort.Width = D3DPresentParams.BackBufferWidth;
	ViewPort.Height = D3DPresentParams.BackBufferHeight;
	ViewPort.X = 0;
	ViewPort.Y = 0;
	ViewPort.MinZ = 0.0f;
	ViewPort.MaxZ = 1.0f;
	pD3DDevice->SetViewport(&ViewPort);

	CurrDepthStencilFormat =
		D3DPresentParams.EnableAutoDepthStencil ?  D3DPresentParams.AutoDepthStencilFormat : D3DFMT_UNKNOWN;

	//!!!CreateQueries();
}
//---------------------------------------------------------------------

void CRenderServer::SetupPresentParams()
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

	CDisplayMode DisplayMode = Display.GetRequestedDisplayMode();
	if (D3DPresentParams.Windowed)
	{
		CDisplayMode DesktopMode;
		n_assert(Display.GetCurrentAdapterDisplayMode((CDisplay::EAdapter)D3DAdapter, DesktopMode));
		DisplayMode.PixelFormat = DesktopMode.PixelFormat;
	}
	else
	{
		if (DisplayMode.PixelFormat == D3DFMT_UNKNOWN)
			DisplayMode.PixelFormat = D3DFMT_X8R8G8B8;

		CArray<CDisplayMode> Modes;
		Display.GetAvailableDisplayModes((CDisplay::EAdapter)D3DAdapter, D3DPresentParams.BackBufferFormat, Modes);
		if (Modes.FindIndex(DisplayMode) == INVALID_INDEX)
		{
			// Find available mode the most close to the requested one
			float IdealAspect = Display.GetDisplayMode().GetAspectRatio();
			float IdealResolution = (float)Display.GetDisplayMode().Width * Display.GetDisplayMode().Height;
			float MinMetric = FLT_MAX;
			int MinIdx = INVALID_INDEX;
			for (int i = 0; i < Modes.GetCount(); ++i)
			{
				const CDisplayMode& Mode = Modes[i];
				float AspectDiff = Mode.GetAspectRatio() - IdealAspect;
				float ResolutionDiff = (float)(Mode.Width * Mode.Height) - IdealResolution;
				float Metric = AspectDiff * AspectDiff + ResolutionDiff * ResolutionDiff;
				if (Metric < MinMetric)
				{
					MinMetric = Metric;
					MinIdx = i;
				}
			}
			n_assert(MinIdx != INVALID_INDEX);
			DisplayMode.Width = Modes[MinIdx].Width;
			DisplayMode.Height = Modes[MinIdx].Height;
		}
	}

	Display.SetDisplayMode(DisplayMode);

	// Make sure the device supports a D24S8 depth buffers
	HRESULT hr = pD3D->CheckDeviceFormat(	D3DAdapter,
											DEM_D3D_DEVICETYPE,
											DisplayMode.PixelFormat,
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
										DisplayMode.PixelFormat,
										DisplayMode.PixelFormat,
										D3DFMT_D24S8);
	if (FAILED(hr))
	{
		n_error("Backbuffer format is not compatible with D24S8 depth buffer!\n");
		return;
	}

	D3DPresentParams.BackBufferWidth = DisplayMode.Width;
	D3DPresentParams.BackBufferHeight = DisplayMode.Height;
	D3DPresentParams.BackBufferFormat = DisplayMode.PixelFormat;
	D3DPresentParams.EnableAutoDepthStencil = TRUE;
	D3DPresentParams.AutoDepthStencilFormat = D3DFMT_D24S8;
	D3DPresentParams.Flags |= D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
}
//---------------------------------------------------------------------

void CRenderServer::SetWireframe(bool Wire)
{
	if (Wireframe == Wire) return;
	Wireframe = Wire;
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
}
//---------------------------------------------------------------------

bool CRenderServer::CheckCaps(ECaps Cap)
{
	n_assert(pD3D && pD3DDevice);

	switch (Cap)
	{
		case Caps_VSTexFiltering_Linear:
			return (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MINFLINEAR) && (D3DCaps.VertexTextureFilterCaps & D3DPTFILTERCAPS_MAGFLINEAR);
		case Caps_VSTex_L16:
			return SUCCEEDED(pD3D->CheckDeviceFormat(	D3DAdapter,
														DEM_D3D_DEVICETYPE,
														D3DPresentParams.BackBufferFormat,
														D3DUSAGE_QUERY_VERTEXTEXTURE,
														D3DRTYPE_TEXTURE,
														D3DFMT_L16));
		default: FAIL;
	}
}
//---------------------------------------------------------------------

bool CRenderServer::BeginFrame()
{
	n_assert(!IsInsideFrame);

	PrimsRendered = 0;
	DIPsRendered = 0;

	//???where? once per frame shader change
	if (!SharedShader.IsValid())
	{
		SharedShader = ShaderMgr.GetTypedResource(CStrID("Shared"));
		n_assert(SharedShader->IsLoaded());
		hLightAmbient = SharedShader->GetVarHandleByName(CStrID("LightAmbient"));
		hEyePos = SharedShader->GetVarHandleByName(CStrID("EyePos"));
		hViewProj = SharedShader->GetVarHandleByName(CStrID("ViewProjection"));
	}

	// CEGUI overwrites this value without restoring it, so restore each frame
	pD3DDevice->SetRenderState(D3DRS_FILLMODE, Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);

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

	CoreSrv->SetGlobal<int>("Render_Prim", PrimsRendered);
	CoreSrv->SetGlobal<int>("Render_DIP", DIPsRendered);
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
		if (FAILED(hr))
		{
			CDisplayMode OldMode = Display.GetDisplayMode();
			if (hr == D3DERR_DEVICELOST) ResetDevice();
			else
			{
				ReleaseDevice();
				if (!CreateDevice()) return;
			}
			if (Display.GetDisplayMode() != OldMode) Display.ResetWindow();
		}
	}    

	++FrameID;

	//// Sync CPU thread with GPU
	//// wait till gpu has finsihed rendering the previous frame
	//gpuSyncQuery[frameId % numSyncQueries]->Issue(D3DISSUE_END);                              
	//++FrameID; //???why here?
	//while (S_FALSE == gpuSyncQuery[frameId % numSyncQueries]->GetData(NULL, 0, D3DGETDATA_FLUSH)) ;
}
//---------------------------------------------------------------------

void CRenderServer::SaveScreenshot(EImageFormat ImageFormat, IO::CStream& OutStream)
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

	if (OutStream.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) //???or open outside? here assert IsOpen and write access
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
	if (CurrRT[Index].GetUnsafe() == pRT) return;

	// Restore main RT to backbuffer and autodepthstencil (or NULL if no auto)
	if (!pRT && Index == 0) pRT = DefaultRT.GetUnsafe();

	IDirect3DSurface9* pRTSurface = pRT ? pRT->GetD3DRenderTargetSurface() : NULL;
	IDirect3DSurface9* pDSSurface = pRT ? pRT->GetD3DDepthStencilSurface() : NULL;

	n_assert(SUCCEEDED(pD3DDevice->SetRenderTarget(Index, pRTSurface)));

	// NB: DS can be set to NULL only by main RT (index 0)
	//???mb set DS only from main RT?
	//???doesn't this kill an auto DS surface?
	if ((pDSSurface || Index == 0) && pDSSurface != pCurrDSSurface)
	{
		CurrDepthStencilFormat = pRT ? pRT->GetDepthStencilFormat() : D3DFMT_UNKNOWN;
		SAFE_RELEASE(pCurrDSSurface);
		pCurrDSSurface = pDSSurface;
		n_assert(SUCCEEDED(pD3DDevice->SetDepthStencilSurface(pDSSurface)));
	}

	CurrRT[Index] = pRT;
}
//---------------------------------------------------------------------

void CRenderServer::SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex)
{
	n_assert(Index < MaxVertexStreamCount && (!pVB || OffsetVertex < pVB->GetVertexCount()));
	if (CurrVB[Index].GetUnsafe() == pVB && CurrVBOffset[Index] == OffsetVertex) return;
	IDirect3DVertexBuffer9* pD3DVB = pVB ? pVB->GetD3DBuffer() : NULL;
	DWORD VertexSize = pVB ? pVB->GetVertexLayout()->GetVertexSize() : 0;
	n_assert(SUCCEEDED(pD3DDevice->SetStreamSource(Index, pD3DVB, VertexSize * OffsetVertex, VertexSize)));
	CurrVB[Index] = pVB;
	CurrVBOffset[Index] = OffsetVertex;
}
//---------------------------------------------------------------------

void CRenderServer::SetVertexLayout(CVertexLayout* pVLayout)
{
	if (CurrVLayout.GetUnsafe() == pVLayout) return;
	IDirect3DVertexDeclaration9* pDecl = pVLayout ? pVLayout->GetD3DVertexDeclaration() : NULL;
	n_assert(SUCCEEDED(pD3DDevice->SetVertexDeclaration(pDecl)));
	CurrVLayout = pVLayout;
}
//---------------------------------------------------------------------

void CRenderServer::SetIndexBuffer(CIndexBuffer* pIB)
{
	if (CurrIB.GetUnsafe() == pIB) return;
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

	D3DPRIMITIVETYPE D3DPrimType;
	DWORD PrimCount = (CurrPrimGroup.IndexCount > 0) ? CurrPrimGroup.IndexCount : CurrPrimGroup.VertexCount;
	switch (CurrPrimGroup.Topology)
	{
		case PointList:	D3DPrimType = D3DPT_POINTLIST; break;
		case LineList:	D3DPrimType = D3DPT_LINELIST; PrimCount /= 2; break;
		case LineStrip:	D3DPrimType = D3DPT_LINESTRIP; --PrimCount; break;
		case TriList:	D3DPrimType = D3DPT_TRIANGLELIST; PrimCount /= 3; break;
		case TriStrip:	D3DPrimType = D3DPT_TRIANGLESTRIP; PrimCount -= 2; break;
		default:		n_error("CRenderServer::Draw() -> Invalid primitive topology!"); return;
	}

	HRESULT hr;
	if (CurrPrimGroup.IndexCount > 0)
	{
		n_assert_dbg(CurrIB.IsValid());
		n_assert_dbg(!InstanceCount || CurrVB[0].IsValid());
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

	PrimsRendered += InstanceCount ? InstanceCount * PrimCount : PrimCount;
	++DIPsRendered;
}
//---------------------------------------------------------------------

PVertexLayout CRenderServer::GetVertexLayout(const CArray<CVertexComponent>& Components)
{
	if (!Components.GetCount()) return NULL;
	CStrID Signature = CVertexLayout::BuildSignature(Components);
	int Idx = VertexLayouts.FindIndex(Signature);
	if (Idx != INVALID_INDEX) return VertexLayouts.ValueAt(Idx);
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

	n_error("CRenderServer::GetPixelFormat() -> Format %s not found!\n", String.CStr());
	return PixelFormat_Invalid;
}
//---------------------------------------------------------------------

//!!!copied from N2, some formats may be missing! Update it!
int CRenderServer::GetFormatBits(EPixelFormat Format)
{
	switch (Format)
	{
		case D3DFMT_A32B32G32R32F:
			return 128;

		case D3DFMT_A16B16G16R16F:
		case D3DFMT_G32R32F:
			return 64;

		case D3DFMT_R8G8B8:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
		case D3DFMT_G16R16:
		case D3DFMT_A4L4:
		case D3DFMT_X8L8V8U8:
		case D3DFMT_Q8W8V8U8:
		case D3DFMT_V16U16:
		case D3DFMT_A2B10G10R10:
		case D3DFMT_A2W10V10U10:
		case D3DFMT_R32F:
		case D3DFMT_G16R16F:
			return 32;

		case D3DFMT_R5G6B5:
		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
		case D3DFMT_A4R4G4B4:
		case D3DFMT_A8R3G3B2:
		case D3DFMT_X4R4G4B4:
		case D3DFMT_A8P8:
		case D3DFMT_A8L8:
		case D3DFMT_V8U8:
		case D3DFMT_L6V5U5:
		case D3DFMT_L16:
		case D3DFMT_R16F:
			return 16;

		case D3DFMT_P8:
		case D3DFMT_A8:
		case D3DFMT_L8:
		case D3DFMT_R3G3B2:
		case D3DFMT_DXT2:
		case D3DFMT_DXT3:
		case D3DFMT_DXT4:
		case D3DFMT_DXT5:
			return 8;

		case D3DFMT_DXT1:
			return 4;

		default:
			return -1;
	}
}
//---------------------------------------------------------------------

bool CRenderServer::OnDisplayPaint(const Events::CEventBase& Event)
{
	if (Display.Fullscreen && pD3DDevice) //!!! && !InDialogBoxMode)
		pD3DDevice->Present(0, 0, 0, 0);
	OK;
}
//---------------------------------------------------------------------

bool CRenderServer::OnToggleFullscreenWindowed(const Events::CEventBase& Event)
{
	Display.Fullscreen = !Display.Fullscreen;
	ResetDevice();
	Display.ResetWindow();
	OK;
}
//---------------------------------------------------------------------

bool CRenderServer::OnDisplaySizeChanged(const Events::CEventBase& Event)
{
	if (Display.Fullscreen) FAIL;
	if (Display.GetDisplayMode().Width != D3DPresentParams.BackBufferWidth ||
		Display.GetDisplayMode().Height != D3DPresentParams.BackBufferHeight)
		ResetDevice();
	OK;
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