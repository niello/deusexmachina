#pragma once
#ifndef __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__

#include <Render/GPUDriver.h>
#include <Render/D3D9/D3D9SwapChain.h>
#include <Data/FixedArray.h>

#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h> // At least for a CAPS structure

// Direct3D9 GPU device driver.
// Multihead (multimonitor) feature is not implemented. You may do it by yourself.
// NB: D3D9 device can't be created without a swap chain, so you MUST call CreateSwapChain()
// before using any device-dependent methods.

namespace Render
{
typedef Ptr<class CD3D9RenderTarget> PD3D9RenderTarget;
typedef Ptr<class CD3D9DepthStencilBuffer> PD3D9DepthStencilBuffer;

class CD3D9GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D9GPUDriver);

protected:

	CFixedArray<PD3D9RenderTarget>	CurrRT;
	PD3D9DepthStencilBuffer			CurrDS;

	CArray<CD3D9SwapChain>			SwapChains;
	bool							IsInsideFrame;
	//bool							Wireframe;

	D3DCAPS9						D3DCaps;
	IDirect3DDevice9*				pD3DDevice;

	Events::PSub					Sub_OnPaint; // Fullscreen-only, so only one swap chain will be subscribed

	CD3D9GPUDriver(): SwapChains(1, 1), pD3DDevice(NULL), IsInsideFrame(false) {}

	// Events are received from swap chain windows, so subscriptions are in swap chains
	bool				OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool				OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	bool				CreateD3DDevice(DWORD CurrAdapterID, EGPUDriverType CurrDriverType, D3DPRESENT_PARAMETERS D3DPresentParams);
	bool				InitSwapChainRenderTarget(CD3D9SwapChain& SC);
	bool				Reset(D3DPRESENT_PARAMETERS& D3DPresentParams, DWORD TargetSwapChainID);
	void				Release();

	void				FillD3DPresentParams(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, const Sys::COSWindow* pWindow, D3DPRESENT_PARAMETERS& D3DPresentParams) const;
	bool				GetCurrD3DPresentParams(const CD3D9SwapChain& SC, D3DPRESENT_PARAMETERS& D3DPresentParams) const;
	static D3DDEVTYPE	GetD3DDriverType(EGPUDriverType DriverType);
	static void			GetUsagePool(DWORD InAccessFlags, DWORD& OutUsage, D3DPOOL& OutPool);

	friend class CD3D9DriverFactory;

public:

	virtual ~CD3D9GPUDriver() { Release(); }

	virtual bool				Init(DWORD AdapterNumber, EGPUDriverType DriverType);
	virtual bool				CheckCaps(ECaps Cap);
	virtual DWORD				GetMaxTextureSize(ETextureType Type);
	virtual DWORD				GetMaxMultipleRenderTargetCount() { return CurrRT.GetCount(); }

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow);
	virtual bool				DestroySwapChain(DWORD SwapChainID);
	virtual bool				SwapChainExists(DWORD SwapChainID) const;
	virtual bool				ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool				SwitchToFullscreen(DWORD SwapChainID, CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL);
	virtual bool				SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL);
	virtual bool				IsFullscreen(DWORD SwapChainID) const;
	virtual PRenderTarget		GetSwapChainRenderTarget(DWORD SwapChainID) const;
	//!!!get info, change info (or only recreate?)
	virtual bool				Present(DWORD SwapChainID);
	//virtual void				SaveScreenshot(DWORD SwapChainID, EImageFormat ImageFormat /*use image codec ref?*/, IO::CStream& OutStream);

	virtual bool				SetViewport(DWORD Index, const CViewport* pViewport); // NULL to reset
	virtual bool				GetViewport(DWORD Index, CViewport& OutViewport);
	virtual bool				SetScissorRect(DWORD Index, const Data::CRect* pScissorRect); // NULL to reset
	virtual bool				GetScissorRect(DWORD Index, Data::CRect& OutScissorRect);

	virtual bool				BeginFrame();
	virtual void				EndFrame();
	virtual bool				SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS);
	virtual void				Clear(DWORD Flags, const vector4& ColorRGBA, float Depth, uchar Stencil);
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA);

	virtual PVertexLayout		CreateVertexLayout() { return NULL; } // Prefer GetVertexLayout() when possible
	virtual PVertexBuffer		CreateVertexBuffer() { return NULL; }
	virtual PIndexBuffer		CreateIndexBuffer() { return NULL; }
	virtual PTexture			CreateTexture(const CTextureDesc& Desc, DWORD AccessFlags, const void* pData = NULL, bool MipDataProvided = false);
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc);
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc);

	//void						SetWireframe(bool Wire);
	//bool						IsWireframe() const { return Wireframe; }

	bool						GetD3DMSAAParams(EMSAAQuality MSAA, D3DFORMAT Format, D3DMULTISAMPLE_TYPE& OutType, DWORD& OutQuality) const;
	IDirect3DDevice9*			GetD3DDevice() const { return pD3DDevice; }
};

typedef Ptr<CD3D9GPUDriver> PD3D9GPUDriver;

}

#endif
