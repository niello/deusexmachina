//#include "RenderServer.h"
//
//#include <Events/EventServer.h>
//#include <IO/Stream.h>
//#include <Core/CoreServer.h>
//#include <dxerr.h>
//
//namespace Render
//{
//__ImplementClassNoFactory(Render::CRenderServer, Core::CObject);
//__ImplementSingleton(CRenderServer);
//
//bool CRenderServer::Open()
//{
//	n_assert(!_IsOpen);
//
//	CDisplayMode OldMode = Display.GetDisplayMode();
//	if (!CreateDevice()) FAIL;
//	if (Display.GetDisplayMode() != OldMode) Display.ResetWindow();
//
//	FFlagSkinned = ShaderFeatures.GetMask("Skinned");
//	FFlagInstanced = ShaderFeatures.GetMask("Instanced");
//
//	if (!DefaultRT.IsValid())
//	{
//		DefaultRT = n_new(CRenderTarget);
//		if (!DefaultRT->CreateDefaultRT()) FAIL;
//	}
//	pCurrDSSurface = DefaultRT->GetD3DDepthStencilSurface();
//
//	n_assert(SUCCEEDED(D3DXCreateEffectPool(&pEffectPool)));
//
//	SUBSCRIBE_PEVENT(OnDisplayPaint, CRenderServer, OnDisplayPaint);
//	SUBSCRIBE_PEVENT(OnDisplayToggleFullscreen, CRenderServer, OnToggleFullscreenWindowed);
//	SUBSCRIBE_PEVENT(OnDisplaySizeChanged, CRenderServer, OnDisplaySizeChanged);
//
//	_IsOpen = true;
//	OK;
//}
////---------------------------------------------------------------------
//
//void CRenderServer::Close()
//{
//	n_assert(_IsOpen);
//
//	UNSUBSCRIBE_EVENT(OnDisplayPaint);
//	UNSUBSCRIBE_EVENT(OnDisplayToggleFullscreen);
//	UNSUBSCRIBE_EVENT(OnDisplaySizeChanged);
//
//	SAFE_RELEASE(pCurrDSSurface);
//	DefaultRT->Destroy();
//	DefaultRT = NULL;
//
//	ReleaseDevice();
//
//	if (Display.IsWindowOpen()) Display.CloseWindow();
//
//	_IsOpen = false;
//}
////---------------------------------------------------------------------
//
//void CRenderServer::SetWireframe(bool Wire)
//{
//	if (Wireframe == Wire) return;
//	Wireframe = Wire;
//	pD3DDevice->SetRenderState(D3DRS_FILLMODE, Wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
//}
////---------------------------------------------------------------------
//
//void CRenderServer::SaveScreenshot(EImageFormat ImageFormat, IO::CStream& OutStream)
//{
//	n_assert(pD3DDevice && !IsInsideFrame);
//
//	IDirect3DSurface9* pCaptureSurface = NULL;
//	HRESULT hr = pD3DDevice->CreateOffscreenPlainSurface(	D3DPresentParams.BackBufferWidth,
//															D3DPresentParams.BackBufferHeight,
//															D3DPresentParams.BackBufferFormat,
//															D3DPOOL_SYSTEMMEM,
//															&pCaptureSurface,
//															NULL);
//	n_assert(SUCCEEDED(hr) && pCaptureSurface);
//
//	// If BackBuffer(0) surface ptr changes, need to update DefaultRT RTSurface ptr every frame
//	// Capturing DefaultRT is better since we always get actual data even if don't render to swap chain
//	n_assert(SUCCEEDED(pD3DDevice->GetRenderTargetData(DefaultRT->GetD3DRenderTargetSurface(), pCaptureSurface)));
//
//	ID3DXBuffer* pBuf = NULL;    
//	hr = D3DXSaveSurfaceToFileInMemory(&pBuf, ImageFormat, pCaptureSurface, NULL, NULL);
//	n_assert(SUCCEEDED(hr));
//	pCaptureSurface->Release();
//
//	if (OutStream.Open(IO::SAM_WRITE, IO::SAP_SEQUENTIAL)) //???or open outside? here assert IsOpen and write access
//	{
//		OutStream.Write(pBuf->GetBufferPointer(), pBuf->GetBufferSize());
//		OutStream.Close();
//	}
//	pBuf->Release();
//}
////---------------------------------------------------------------------
//
//void CRenderServer::SetRenderTarget(DWORD Index, CRenderTarget* pRT)
//{
//	n_assert(Index < MaxRenderTargetCount);
//	if (CurrRT[Index].GetUnsafe() == pRT) return;
//
//	// Restore main RT to backbuffer and autodepthstencil (or NULL if no auto)
//	if (!pRT && Index == 0) pRT = DefaultRT.GetUnsafe();
//
//	IDirect3DSurface9* pRTSurface = pRT ? pRT->GetD3DRenderTargetSurface() : NULL;
//	IDirect3DSurface9* pDSSurface = pRT ? pRT->GetD3DDepthStencilSurface() : NULL;
//
//	n_assert(SUCCEEDED(pD3DDevice->SetRenderTarget(Index, pRTSurface)));
//
//	// NB: DS can be set to NULL only by main RT (index 0)
//	//???mb set DS only from main RT?
//	//???doesn't this kill an auto DS surface?
//	if ((pDSSurface || Index == 0) && pDSSurface != pCurrDSSurface)
//	{
//		CurrDepthStencilFormat = pRT ? pRT->GetDepthStencilFormat() : D3DFMT_UNKNOWN;
//		SAFE_RELEASE(pCurrDSSurface);
//		pCurrDSSurface = pDSSurface;
//		n_assert(SUCCEEDED(pD3DDevice->SetDepthStencilSurface(pDSSurface)));
//	}
//
//	CurrRT[Index] = pRT;
//}
////---------------------------------------------------------------------
//
//void CRenderServer::SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex)
//{
//	n_assert(Index < MaxVertexStreamCount && (!pVB || OffsetVertex < pVB->GetVertexCount()));
//	if (CurrVB[Index].GetUnsafe() == pVB && CurrVBOffset[Index] == OffsetVertex) return;
//	IDirect3DVertexBuffer9* pD3DVB = pVB ? pVB->GetD3DBuffer() : NULL;
//	DWORD VertexSize = pVB ? pVB->GetVertexLayout()->GetVertexSize() : 0;
//	n_assert(SUCCEEDED(pD3DDevice->SetStreamSource(Index, pD3DVB, VertexSize * OffsetVertex, VertexSize)));
//	CurrVB[Index] = pVB;
//	CurrVBOffset[Index] = OffsetVertex;
//}
////---------------------------------------------------------------------
//
//void CRenderServer::SetVertexLayout(CVertexLayout* pVLayout)
//{
//	if (CurrVLayout.GetUnsafe() == pVLayout) return;
//	IDirect3DVertexDeclaration9* pDecl = pVLayout ? pVLayout->GetD3DVertexDeclaration() : NULL;
//	n_assert(SUCCEEDED(pD3DDevice->SetVertexDeclaration(pDecl)));
//	CurrVLayout = pVLayout;
//}
////---------------------------------------------------------------------
//
//void CRenderServer::SetIndexBuffer(CIndexBuffer* pIB)
//{
//	if (CurrIB.GetUnsafe() == pIB) return;
//	IDirect3DIndexBuffer9* pD3DIB = pIB ? pIB->GetD3DBuffer() : NULL;
//	n_assert(SUCCEEDED(pD3DDevice->SetIndices(pD3DIB)));
//	CurrIB = pIB;
//}
////---------------------------------------------------------------------
//
//// Docs: Note that D3DSTREAMSOURCE_INDEXEDDATA and the number of instances to draw must always be set in stream zero.
//void CRenderServer::SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex)
//{
//	n_assert2(!pVB || Instances, "CRenderServer::SetInstanceBuffer() -> Specify 1 or more instances!");
//	n_assert(Index > 0); //???force instance buffer Index to be always the same (1 or smth)?
//
//	SetVertexBuffer(Index, pVB, OffsetVertex);
//
//	DWORD NewInstanceCount = pVB ? Instances : 0;
//	if (NewInstanceCount == InstanceCount) return;
//	InstanceCount = NewInstanceCount;
//
//	if (InstanceCount > 0)
//	{
//		pD3DDevice->SetStreamSourceFreq(0, D3DSTREAMSOURCE_INDEXEDDATA | InstanceCount);
//		pD3DDevice->SetStreamSourceFreq(Index, D3DSTREAMSOURCE_INSTANCEDATA | 1);
//	}
//	else
//	{
//		pD3DDevice->SetStreamSourceFreq(0, 1);
//		pD3DDevice->SetStreamSourceFreq(Index, 1);
//	}
//}
////---------------------------------------------------------------------
//
//void CRenderServer::Draw()
//{
//	n_assert_dbg(pD3DDevice && IsInsideFrame);
//
//	D3DPRIMITIVETYPE D3DPrimType;
//	DWORD PrimCount = (CurrPrimGroup.IndexCount > 0) ? CurrPrimGroup.IndexCount : CurrPrimGroup.VertexCount;
//	switch (CurrPrimGroup.Topology)
//	{
//		case PointList:	D3DPrimType = D3DPT_POINTLIST; break;
//		case LineList:	D3DPrimType = D3DPT_LINELIST; PrimCount /= 2; break;
//		case LineStrip:	D3DPrimType = D3DPT_LINESTRIP; --PrimCount; break;
//		case TriList:	D3DPrimType = D3DPT_TRIANGLELIST; PrimCount /= 3; break;
//		case TriStrip:	D3DPrimType = D3DPT_TRIANGLESTRIP; PrimCount -= 2; break;
//		default:		Sys::Error("CRenderServer::Draw() -> Invalid primitive topology!"); return;
//	}
//
//	HRESULT hr;
//	if (CurrPrimGroup.IndexCount > 0)
//	{
//		n_assert_dbg(CurrIB.IsValid());
//		n_assert_dbg(!InstanceCount || CurrVB[0].IsValid());
//		hr = pD3DDevice->DrawIndexedPrimitive(	D3DPrimType,
//												0,
//												CurrPrimGroup.FirstVertex,
//												CurrPrimGroup.VertexCount,
//												CurrPrimGroup.FirstIndex,
//												PrimCount);
//	}
//	else
//	{
//		n_assert2_dbg(!InstanceCount, "Non-indexed instanced rendereng is not supported by design!");
//		hr = pD3DDevice->DrawPrimitive(D3DPrimType, CurrPrimGroup.FirstVertex, PrimCount);
//	}
//	n_assert(SUCCEEDED(hr));
//
//	PrimsRendered += InstanceCount ? InstanceCount * PrimCount : PrimCount;
//	++DIPsRendered;
//}
////---------------------------------------------------------------------
//
//EPixelFormat CRenderServer::GetPixelFormat(const CString& String)
//{
//	if (String.IsEmpty()) return PixelFormat_Invalid;
//
//	//!!!PERF!
//	//!!!REWRITE in more elegant and optimal way!
//	if (String == "X8R8G8B8") return D3DFMT_X8R8G8B8;
//	if (String == "A8R8G8B8") return D3DFMT_A8R8G8B8;
//	if (String == "R5G6B5") return D3DFMT_R5G6B5;
//	if (String == "R16F") return D3DFMT_R16F;
//	if (String == "G16R16F") return D3DFMT_G16R16F;
//	if (String == "A16B16G16R16F") return D3DFMT_A16B16G16R16F;
//	if (String == "R32F") return D3DFMT_R32F;
//	if (String == "G32R32F") return D3DFMT_G32R32F;
//	if (String == "A32B32G32R32F") return D3DFMT_A32B32G32R32F;
//	if (String == "D32") return D3DFMT_D32;
//	if (String == "D15S1") return D3DFMT_D15S1;
//	if (String == "D24S8") return D3DFMT_D24S8;
//	if (String == "D24X8") return D3DFMT_D24X8;
//	if (String == "D24X4S4") return D3DFMT_D24X4S4;
//
//	Sys::Error("CRenderServer::GetPixelFormat() -> Format %s not found!\n", String.CStr());
//	return PixelFormat_Invalid;
//}
////---------------------------------------------------------------------
//
///*
//void D3D9RenderDevice::UnbindD3D9Resources()
//{
//    d3d9Device->SetVertexShader(NULL);
//    d3d9Device->SetPixelShader(NULL);
//    for (int i = 0; i < MaxTextureStageCount; ++i)
//        d3d9Device->SetTexture(i, NULL);
//    for (int i = 0; i < MaxNumVertexStreams; ++i)
//		SetVertexBuffer(i, NULL);
//    SetIndexBuffer(NULL);
//}
////---------------------------------------------------------------------
////!!!my device has no EndPass!
//D3D9RenderDevice::EndPass()
//{
//	UnbindD3D9Resources();
//	RenderDeviceBase::EndPass();
//}
////---------------------------------------------------------------------
//void D3D9RenderDevice::DiscardQueries()
//{
//	for (IndexT i = 0; i < numSyncQueries; ++i)
//		SAFE_RELEASE(gpuSyncQuery[i]);  	
//}
////------------------------------------------------------------------------------
//void D3D9RenderDevice::SetupQueries()
//{
//	// create double buffer query to avoid gpu to render more than 1 frame ahead
//	for (int i = 0; i < numSyncQueries; ++i)
//		d3d9Device->CreateQuery(D3DQUERYTYPE_EVENT, &gpuSyncQuery[i]);  	
//}
//*/
//}