#pragma once
#ifndef __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__

#include <Render/GPUDriver.h>

// Direct3D11 GPU device driver.

struct IDXGISwapChain;
//struct ID3D11Texture2D;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace Render
{
typedef Ptr<class CD3D11RenderState> PD3D11RenderState;

class CD3D11GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D11GPUDriver);

protected:

	//???store index and driver pointer and subscribe events here?
	class CD3D11SwapChain: public CSwapChain
	{
	public:

		//???need? DXGI handles OS window!
		Events::PSub		Sub_OnToggleFullscreen;
		Events::PSub		Sub_OnSizeChanged;
		Events::PSub		Sub_OnClosing;

		IDXGISwapChain*		pSwapChain;
		//ID3D11Texture2D*	pBackBuffer;
		//!!!RT View! or unify RT of swap chain and texture?

		void Release();
	};

	CArray<CD3D11SwapChain>		SwapChains;
	//bool						IsInsideFrame;
	//bool						Wireframe;

	ID3D11Device*				pD3DDevice;
	ID3D11DeviceContext*		pD3DImmContext;
	//???store also D3D11.1 interfaces? an use for 11.1 methods only.

	CArray<PD3D11RenderState>	RenderStates;

	CD3D11GPUDriver(): SwapChains(1, 1)/*, IsInsideFrame(false)*/ {}

	////???subscribe swapchain itself? more handling and self-control into a swapchain class?
	//bool			OnOSWindowToggleFullscreen(const Events::CEventBase& Event);
	//bool			OnOSWindowSizeChanged(const Events::CEventBase& Event);
	//bool			OnOSWindowPaint(const Events::CEventBase& Event);
	//bool			OnOSWindowClosing(const Events::CEventBase& Event);

	void			Release();

	//static void		FillD3DPresentParams(const CSwapChainDesc& Desc, const Sys::COSWindow* pWindow, D3DPRESENT_PARAMETERS& D3DPresentParams);
	//static bool		GetCurrD3DPresentParams(const CSwapChain& SC, D3DPRESENT_PARAMETERS& D3DPresentParams);

	friend class CD3D11DriverFactory;

	virtual PVertexLayout	InternalCreateVertexLayout();

public:

	virtual ~CD3D11GPUDriver() {}

	virtual bool			Init(DWORD AdapterNumber);
	virtual bool			CheckCaps(ECaps Cap);

	virtual DWORD			CreateSwapChain(const CSwapChainDesc& Desc, Sys::COSWindow* pWindow);
	virtual bool			DestroySwapChain(DWORD SwapChainID);
	virtual bool			SwapChainExists(DWORD SwapChainID) const;
	virtual bool			ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool			SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL);
	virtual bool			SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL);
	virtual bool			IsFullscreen(DWORD SwapChainID) const;
	//!!!get info, change info (or only recreate?)
	virtual bool			Present(DWORD SwapChainID);
	//virtual void			SaveScreenshot(DWORD SwapChainID, EImageFormat ImageFormat /*use image codec ref?*/, IO::CStream& OutStream);

	virtual PVertexLayout	CreateVertexLayout(); // Prefer GetVertexLayout() when possible
	virtual PVertexBuffer	CreateVertexBuffer();
	virtual PIndexBuffer	CreateIndexBuffer();
	virtual PRenderState	CreateRenderState(const Data::CParams& Desc);

	//void					SetWireframe(bool Wire);
	//bool					IsWireframe() const { return Wireframe; }

	//IDirect3DDevice9*		GetD3DDevice() const { return pD3DDevice; }
};

}

#endif
