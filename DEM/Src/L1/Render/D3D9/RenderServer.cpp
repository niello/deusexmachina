//bool CRenderServer::Open()
//{
//	FFlagSkinned = ShaderFeatures.GetMask("Skinned");
//	FFlagInstanced = ShaderFeatures.GetMask("Instanced");
//
//	n_assert(SUCCEEDED(D3DXCreateEffectPool(&pEffectPool)));
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
//void D3D9RenderDevice::UnbindD3DResources()
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
