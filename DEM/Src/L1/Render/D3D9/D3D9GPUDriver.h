#pragma once
#ifndef __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D9_GPU_DRIVER_H__

#include <Render/GPUDriver.h>

#define WIN32_LEAN_AND_MEAN
#define D3D_DISABLE_9EX
#include <d3d9.h> // At least for a CAPS structure

// Direct3D9 GPU device driver

struct ID3DXEffectPool; // D3DX

namespace Render
{

class CD3D9GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D9GPUDriver);

protected:

	//???store index and driver pointer and subscribe events here?
	class CSwapChain: public CSwapChainBase
	{
	public:

		Events::PSub	Sub_OnToggleFullscreen;
		Events::PSub	Sub_OnSizeChanged;
		Events::PSub	Sub_OnPaint;
		Events::PSub	Sub_OnClosing;

		//???store present params along with a desc, or recreate from scratch each time?
		IDirect3DSwapChain9* pSwapChain; // NULL for implicit swap chain, device methods will be called
	};

	CArray<CSwapChain>	SwapChains;

	D3DCAPS9			D3DCaps;
	IDirect3DDevice9*	pD3DDevice;
	ID3DXEffectPool*	pEffectPool;

	CD3D9GPUDriver(): SwapChains(1, 1) {}

	//???subscribe swapchain itself? more handling and self-control into a swapchain class?
	bool			OnOSWindowToggleFullscreen(const Events::CEventBase& Event);
	bool			OnOSWindowSizeChanged(const Events::CEventBase& Event);
	bool			OnOSWindowPaint(const Events::CEventBase& Event);
	bool			OnOSWindowClosing(const Events::CEventBase& Event);

	bool			Reset(D3DPRESENT_PARAMETERS& D3DPresentParams);
	void			Release();

	static void		FillD3DPresentParams(const CSwapChainDesc& Desc, const Sys::COSWindow* pWindow, D3DPRESENT_PARAMETERS& D3DPresentParams);

	friend class CD3D9DriverFactory;

public:

	virtual ~CD3D9GPUDriver() {}

	//virtual bool			Init(DWORD AdapterNumber); // Use CreateSwapChain() to create device with implicit swap chain
	virtual bool			CheckCaps(ECaps Cap);

	virtual DWORD			CreateSwapChain(const CSwapChainDesc& Desc, Sys::COSWindow* pWindow);
	virtual bool			DestroySwapChain(DWORD SwapChainID);
	virtual bool			SwapChainExists(DWORD SwapChainID) const;
	virtual bool			SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL);
	virtual bool			SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL);
	virtual bool			ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool			IsFullscreen(DWORD SwapChainID) const;
	//!!!get info, change info (or only recreate?)
	virtual void			Present(DWORD SwapChainID);
	virtual void			PresentBlankScreen(DWORD SwapChainID, DWORD Color);
	//virtual void			SaveScreenshot(DWORD SwapChainID, EImageFormat ImageFormat /*use image codec ref?*/, IO::CStream& OutStream);

	virtual PVertexLayout	CreateVertexLayout(); // Prefer GetVertexLayout() when possible
	virtual PVertexBuffer	CreateVertexBuffer();
	virtual PIndexBuffer	CreateIndexBuffer();

	IDirect3DDevice9*		GetD3DDevice() const { return pD3DDevice; }
	ID3DXEffectPool*		GetD3DEffectPool() const { return pEffectPool; }
};

}

#endif
