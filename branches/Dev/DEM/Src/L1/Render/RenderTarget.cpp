#include "RenderTarget.h"

#include <Render/RenderServer.h>

namespace Render
{

//???bool Windowed param?
inline void CRenderTarget::GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT RTFormat, D3DFORMAT DSFormat,
											D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality)
{
#if DEM_D3D_DEBUG
	OutType = D3DMULTISAMPLE_NONE;
	OutQuality = 0;
#else
	switch (MSAA)
	{
		case MSAA_None:	OutType = D3DMULTISAMPLE_NONE; break;
		case MSAA_2x:	OutType = D3DMULTISAMPLE_2_SAMPLES; break;
		case MSAA_4x:	OutType = D3DMULTISAMPLE_4_SAMPLES; break;
		case MSAA_8x:	OutType = D3DMULTISAMPLE_8_SAMPLES; break;
	};

	DWORD QualLevels = 0;
	HRESULT hr = RenderSrv->GetD3D()->CheckDeviceMultiSampleType(	RenderSrv->GetD3DAdapter(),
																	DEM_D3D_DEVICETYPE,
																	RTFormat,
																	FALSE,
																	OutType,
																	&QualLevels);
	if (hr == D3DERR_NOTAVAILABLE)
	{
		OutType = D3DMULTISAMPLE_NONE;
		OutQuality = 0;
		return;
	}
	n_assert(SUCCEEDED(hr));

	OutQuality = QualLevels ? QualLevels - 1 : 0;

	if (DSFormat == D3DFMT_UNKNOWN) return;

	hr = RenderSrv->GetD3D()->CheckDeviceMultiSampleType(	RenderSrv->GetD3DAdapter(),
															DEM_D3D_DEVICETYPE,
															DSFormat,
															FALSE,
															OutType,
															NULL);
	if (hr == D3DERR_NOTAVAILABLE)
	{
		OutType = D3DMULTISAMPLE_NONE;
		OutQuality = 0;
		return;
	}
	n_assert(SUCCEEDED(hr));
#endif
}
//---------------------------------------------------------------------

bool CRenderTarget::CreateDefaultRT()
{
	//???assert not created?

	IsDefaultRT = true;

	//const CDisplayMode& DispMode = RenderSrv->GetDisplay().GetDisplayMode();
	//float Width = DispMode.Width;
	//float Height = DispMode.Height;
	//EPixelFormat Fornat = DispMode.PixelFormat;
	//EMSAAQuality MSAA = RenderSrv->GetDisplay().AntiAliasQuality;

	RTTexture = NULL;
	n_assert(SUCCEEDED(RenderSrv->GetD3DDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pRTSurface)));
	n_assert(SUCCEEDED(RenderSrv->GetD3DDevice()->GetDepthStencilSurface(&pDSSurface)));

	OK;
}
//---------------------------------------------------------------------

bool CRenderTarget::Create(CStrID TextureID, D3DFORMAT RTFormat, D3DFORMAT DSFormat, float Width, float Height,
						   bool AbsWH, EMSAAQuality MSAA, DWORD TexWidth, DWORD TexHeight
						   /*, use mips, shared DS*/)
{
	if (!TextureID.IsValid() || RTFormat == D3DFMT_UNKNOWN) FAIL;

	//???assert not created?

	IsDefaultRT = false;

	//???members?
	DWORD AbsWidth;
	DWORD AbsHeight;

	if (AbsWH)
	{
		AbsWidth = (DWORD)Width;
		AbsHeight = (DWORD)Height;
	}
	else
	{
		const CDisplayMode& DispMode = RenderSrv->GetDisplay().GetDisplayMode();
		AbsWidth = (DWORD)(Width * (float)DispMode.Width);
		AbsHeight = (DWORD)(Height * (float)DispMode.Height);
	}

	RTTexture = RenderSrv->TextureMgr.GetTypedResource(TextureID);
	n_assert2(!RTTexture->IsLoaded(), "Render target specifies TextureID of already loaded texture");

	D3DMULTISAMPLE_TYPE D3DMSAAType;
	DWORD D3DMSAAQuality;
	GetD3DMSAAParams(MSAA, RTFormat, DSFormat, D3DMSAAType, D3DMSAAQuality);

	ResolveToTexture = (D3DMSAAType != D3DMULTISAMPLE_NONE) ||
		(TexWidth && TexWidth != AbsWidth) ||
		(TexHeight && TexHeight != AbsHeight); // || use mips

	TexWidth = TexWidth ? TexWidth : AbsWidth;
	TexHeight = TexHeight ? TexHeight : AbsHeight;

	if (ResolveToTexture)
	{
		HRESULT hr = RenderSrv->GetD3DDevice()->CreateRenderTarget(	AbsWidth, AbsHeight, RTFormat,
																	D3DMSAAType, D3DMSAAQuality,
																	FALSE, &pRTSurface, NULL);
		n_assert(SUCCEEDED(hr));
        //if (mipMapsEnabled) usage |= D3DUSAGE_AUTOGENMIPMAP; //???is applicable to 1-level RT texture? N3 code.
	}

	n_assert(RTTexture->CreateRenderTarget(RTFormat, TexWidth, TexHeight));

	if (!ResolveToTexture)
		n_assert(SUCCEEDED(RTTexture->GetD3D9Texture()->GetSurfaceLevel(0, &pRTSurface)));

	if (DSFormat != D3DFMT_UNKNOWN)
	{
		HRESULT hr = RenderSrv->GetD3DDevice()->CreateDepthStencilSurface(AbsWidth, AbsHeight, DSFormat,
																		  D3DMSAAType, D3DMSAAQuality, TRUE,
																		  &pDSSurface, NULL);
		n_assert(SUCCEEDED(hr));
	}

	OK;
}
//---------------------------------------------------------------------

void CRenderTarget::Destroy()
{
	SAFE_RELEASE(pRTSurface);
	SAFE_RELEASE(pDSSurface); //???what if shared? may AddRef in Create
	if (RTTexture.isvalid() && RTTexture->IsLoaded()) RTTexture->Unload();
	RTTexture = NULL;
}
//---------------------------------------------------------------------


/*
void
D3D9RenderTarget::BeginPass()
{
    HRESULT hr;
    IDirect3DDevice9* d3d9Dev = D3D9RenderDevice::Instance()->GetDirect3DDevice();

    // apply the render target (may be the back buffer)
    if (0 != this->d3d9RenderTarget)
    {
        d3d9Dev->SetRenderTarget(this->mrtIndex, this->d3d9RenderTarget);
    }
    else
    {
        n_assert(this->IsDefaultRenderTarget());
        IDirect3DSurface9* backBuffer = 0;
        hr = d3d9Dev->GetBackBuffer(0,0, D3DBACKBUFFER_TYPE_MONO, &backBuffer);
        n_assert(SUCCEEDED(hr));
        hr = d3d9Dev->SetRenderTarget(this->mrtIndex, backBuffer);
        n_assert(SUCCEEDED(hr));
        backBuffer->Release();        
    }

    // apply the depth stencil buffer
    if (0 != this->d3d9DepthStencil)
    {
        hr = d3d9Dev->SetDepthStencilSurface(this->d3d9DepthStencil);
        n_assert(SUCCEEDED(hr));
    }
    else
    {
        // for default rendertarget, 
        // always clear depthstencil which could be set in previous offscreen passes
        // this avoids use of possible MSAA depthstencil surface with a always Non-MSAA Backbuffer
        if (this->IsDefaultRenderTarget())
        {
            if (DisplayDevice::Instance()->GetAntiAliasQuality() != AntiAliasQuality::None)
            {
                d3d9Dev->SetDepthStencilSurface(NULL);            
            }               
        }        
    }

    RenderTargetBase::BeginPass();
    
    // clear render target if requested
    DWORD d3d9ClearFlags = 0;
    if (0 != (this->clearFlags & ClearColor))
    {
        d3d9ClearFlags |= D3DCLEAR_TARGET;
    }
    if (0 != this->d3d9DepthStencil)
    {
        if (0 != (this->clearFlags & ClearDepth))
        {
            d3d9ClearFlags |= D3DCLEAR_ZBUFFER;
        }
        if (0 != (this->clearFlags & ClearStencil))
        {
            d3d9ClearFlags |= D3DCLEAR_STENCIL;
        }
    }
    if (0 != d3d9ClearFlags)
    {
        hr = d3d9Dev->Clear(0,             // Count
                            NULL,          // pRects
                            d3d9ClearFlags,    // Flags
                            D3DCOLOR_COLORVALUE(this->clearColor.x(), 
                                this->clearColor.y(), 
                                this->clearColor.z(), 
                                this->clearColor.w()),  // Color
                            this->clearDepth,           // Z
                            this->clearStencil);        // Stencil
        n_assert(SUCCEEDED(hr));
    }

    // get shared pixel size
    if (!this->sharedPixelSize.isvalid())
    {
        ShaderVariable::Semantic semPixelSize(NEBULA3_SEMANTIC_PIXELSIZE);
        this->sharedPixelSize = CoreGraphics::ShaderServer::Instance()->GetSharedVariableBySemantic(semPixelSize);
        n_assert(this->sharedPixelSize.isvalid());
    }
    SizeT w = this->GetWidth();
    SizeT h = this->GetHeight();
    Math::float4 pixelSize(1.0f / float(w), 1.0f / float(h), 0.0f, 0.0f);
    this->sharedPixelSize->SetFloat4(pixelSize);
      
    // get shared half pixel size
    if (!this->sharedHalfPixelSize.isvalid())
    {
        ShaderVariable::Semantic semPixelSize(NEBULA3_SEMANTIC_HALFPIXELSIZE);
        this->sharedHalfPixelSize = CoreGraphics::ShaderServer::Instance()->GetSharedVariableBySemantic(semPixelSize);
        n_assert(this->sharedHalfPixelSize.isvalid());
    }
    // set half pixel size for lookup in pixelshaders into screen mapped textures 
    float xHalfSize = 0.5f / float(w);
    float yHalfSize = 0.5f / float(h);
    this->sharedHalfPixelSize->SetFloat4(Math::float4(xHalfSize, yHalfSize,0,0));
}

//------------------------------------------------------------------------------

void
D3D9RenderTarget::EndPass()
{
    HRESULT hr;
    IDirect3DDevice9* d3d9Dev = D3D9RenderDevice::Instance()->GetDirect3DDevice();

    // if necessary need to resolve the render target, either
    // into our resolve texture, or into the back buffer
    if (this->needsResolve)
    {        
        RECT destRect;
        CONST RECT* pDestRect = NULL;
        if (this->resolveRectValid)
        {
            destRect.left   = this->resolveRect.left;
            destRect.right  = this->resolveRect.right;
            destRect.top    = this->resolveRect.top;
            destRect.bottom = this->resolveRect.bottom;
            pDestRect = &destRect;
        }
        IDirect3DSurface9* resolveSurface = 0;
        hr = this->d3d9ResolveTexture->GetSurfaceLevel(0, &resolveSurface);
        hr = d3d9Dev->StretchRect(this->d3d9RenderTarget, NULL, resolveSurface, pDestRect, D3DTEXF_NONE);
        n_assert(SUCCEEDED(hr));
        
        // need cpu access, copy from gpu mem to sys mem
        if (this->resolveCpuAccess)
        {
            HRESULT hr;   
            D3DLOCKED_RECT dstLockRect;
            D3DLOCKED_RECT srcLockRect;
            IDirect3DSurface9* dstSurface = 0;
            hr = this->d3d9CPUResolveTexture->GetSurfaceLevel(0, &dstSurface);
            n_assert(SUCCEEDED(hr));
            hr = dstSurface->LockRect(&dstLockRect, 0, 0);
            n_assert(SUCCEEDED(hr));
            hr = resolveSurface->LockRect(&srcLockRect, 0, D3DLOCK_READONLY);
            n_assert(SUCCEEDED(hr));
            Memory::Copy(srcLockRect.pBits, dstLockRect.pBits, dstLockRect.Pitch * this->resolveCPUTexture->GetWidth());
            dstSurface->Release();
        }
        resolveSurface->Release();
    }
    else if (this->resolveCpuAccess)
    {
        HRESULT hr;
        // copy data
        IDirect3DSurface9* resolveSurface = 0;
        hr = this->d3d9CPUResolveTexture->GetSurfaceLevel(0, &resolveSurface);
        n_assert(SUCCEEDED(hr));
        hr = d3d9Dev->GetRenderTargetData(this->d3d9RenderTarget, resolveSurface);
        n_assert(SUCCEEDED(hr));
        resolveSurface->Release();
    }
    // unset multiple rendertargets
    if (this->mrtIndex > 0)
    {
        d3d9Dev->SetRenderTarget(this->mrtIndex, 0);
    }    
    RenderTargetBase::EndPass();
}

void
D3D9RenderTarget::OnLostDevice()
{
	if (!this->isLosted)
	{
		this->isLosted = true;
		this->Discard();
	}
}

//------------------------------------------------------------------------------
void
D3D9RenderTarget::OnResetDevice()
{
	if (this->isLosted)
	{
		this->Setup();
		this->isLosted = false;
	}
}

*/
}