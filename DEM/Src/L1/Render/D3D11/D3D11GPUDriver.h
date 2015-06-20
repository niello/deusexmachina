#pragma once
#ifndef __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__

#include <Render/GPUDriver.h>
#include <Render/D3D11/D3D11SwapChain.h>

// Direct3D11 GPU device driver.

struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11DeviceContext;
typedef enum D3D_DRIVER_TYPE D3D_DRIVER_TYPE;

namespace Render
{
typedef Ptr<class CD3D11RenderState> PD3D11RenderState;

class CD3D11GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D11GPUDriver);

protected:

	CArray<CD3D11SwapChain>		SwapChains;
	//bool						IsInsideFrame;
	//bool						Wireframe;

	ID3D11Device*				pD3DDevice;
	ID3D11DeviceContext*		pD3DImmContext;
	//???store also D3D11.1 interfaces? an use for 11.1 methods only.

	CArray<PD3D11RenderState>	RenderStates;

	CD3D11GPUDriver(): SwapChains(1, 1)/*, IsInsideFrame(false)*/ {}

	////???subscribe swapchain itself? more handling and self-control into a swapchain class?
	//bool			OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	//bool			OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	//bool			OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	//bool			OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	void			Release();

	static D3D_DRIVER_TYPE	GetD3DDriverType(EGPUDriverType DriverType);
	static EGPUDriverType	GetDEMDriverType(D3D_DRIVER_TYPE DriverType);

	virtual PVertexLayout	InternalCreateVertexLayout();

	friend class CD3D11DriverFactory;

public:

	virtual ~CD3D11GPUDriver() {}

	virtual bool				Init(DWORD AdapterNumber, EGPUDriverType DriverType);
	virtual bool				CheckCaps(ECaps Cap);

	virtual DWORD				CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow);
	virtual bool				DestroySwapChain(DWORD SwapChainID);
	virtual bool				SwapChainExists(DWORD SwapChainID) const;
	virtual bool				ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool				SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL);
	virtual bool				SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL);
	virtual bool				IsFullscreen(DWORD SwapChainID) const;
	virtual PRenderTarget		GetSwapChainRenderTarget(DWORD SwapChainID) const;
	//!!!get info, change info (or only recreate?)
	virtual bool				Present(DWORD SwapChainID);
	//virtual void				SaveScreenshot(DWORD SwapChainID, EImageFormat ImageFormat /*use image codec ref?*/, IO::CStream& OutStream);
	//PRenderTarget GetSwapChainRenderTarget(DWORD SwapChainID);

	virtual PVertexLayout		CreateVertexLayout(); // Prefer GetVertexLayout() when possible
	virtual PVertexBuffer		CreateVertexBuffer();
	virtual PIndexBuffer		CreateIndexBuffer();
	virtual PRenderState		CreateRenderState(const Data::CParams& Desc);
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc);
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc);

	//void					SetWireframe(bool Wire);
	//bool					IsWireframe() const { return Wireframe; }

	//IDirect3DDevice9*		GetD3DDevice() const { return pD3DDevice; }
};

typedef Ptr<CD3D11GPUDriver> PD3D11GPUDriver;

}

#endif
