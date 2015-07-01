#pragma once
#ifndef __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__
#define __DEM_L1_RENDER_D3D11_GPU_DRIVER_H__

#include <Render/GPUDriver.h>
#include <Render/D3D11/D3D11SwapChain.h>
#include <Data/FixedArray.h>

// Direct3D11 GPU device driver.

struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11DeviceContext;
typedef enum D3D_DRIVER_TYPE D3D_DRIVER_TYPE;
enum D3D11_USAGE;

namespace Render
{
typedef Ptr<class CD3D11RenderTarget> PD3D11RenderTarget;
typedef Ptr<class CD3D11DepthStencilBuffer> PD3D11DepthStencilBuffer;
typedef Ptr<class CD3D11RenderState> PD3D11RenderState;

class CD3D11GPUDriver: public CGPUDriver
{
	__DeclareClass(CD3D11GPUDriver);

public:

	enum
	{
		GPU_Dirty_RT = 0x0001,
		GPU_Dirty_DS = 0x0002,

		GPU_Dirty_All = -1 // All bits set, for convenience in ApplyChanges() call
	};

protected:

	CFixedArray<PD3D11RenderTarget>	CurrRT;
	PD3D11DepthStencilBuffer		CurrDS;
	Data::CFlags					CurrDirtyFlags;

	CArray<CD3D11SwapChain>			SwapChains;
	//bool							IsInsideFrame;
	//bool							Wireframe;

	ID3D11Device*					pD3DDevice;
	ID3D11DeviceContext*			pD3DImmContext;
	//???store also D3D11.1 interfaces? and use for 11.1 methods only.

	CArray<PD3D11RenderState>		RenderStates;

	CD3D11GPUDriver(): SwapChains(1, 1), pD3DDevice(NULL), pD3DImmContext(NULL) /*, IsInsideFrame(false)*/ {}

	bool			OnOSWindowClosing(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnOSWindowSizeChanged(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	bool			OnOSWindowToggleFullscreen(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);
	//bool			OnOSWindowPaint(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event);

	bool			InitSwapChainRenderTarget(CD3D11SwapChain& SC);
	void			Release();

	static D3D_DRIVER_TYPE	GetD3DDriverType(EGPUDriverType DriverType);
	static EGPUDriverType	GetDEMDriverType(D3D_DRIVER_TYPE DriverType);
	static void				GetUsageAccess(DWORD InAccessFlags, bool InitDataProvided, D3D11_USAGE& OutUsage, UINT& OutCPUAccess);

	virtual PVertexLayout	InternalCreateVertexLayout();

	friend class CD3D11DriverFactory;

public:

	virtual ~CD3D11GPUDriver() { Release(); }

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

	virtual bool				BeginFrame();
	virtual void				EndFrame();
	virtual bool				SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	virtual bool				SetDepthStencilBuffer(CDepthStencilBuffer* pDS);
	virtual void				Clear(DWORD Flags, const vector4& ColorRGBA, float Depth, uchar Stencil);
	virtual void				ClearRenderTarget(CRenderTarget& RT, const vector4& ColorRGBA);

	DWORD						ApplyChanges(DWORD ChangesToUpdate = GPU_Dirty_All); // returns a combination of dirty flags where errors occured

	virtual PVertexLayout		CreateVertexLayout() { return NULL; } // Prefer GetVertexLayout() when possible
	virtual PVertexBuffer		CreateVertexBuffer() { return NULL; }
	virtual PIndexBuffer		CreateIndexBuffer() { return NULL; }
	virtual PRenderState		CreateRenderState(const Data::CParams& Desc);
	virtual PTexture			CreateTexture(const CTextureDesc& Desc, DWORD AccessFlags, const void* pData = NULL, bool MipDataProvided = false);
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc);
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc);

	//void					SetWireframe(bool Wire);
	//bool					IsWireframe() const { return Wireframe; }

	//IDirect3DDevice9*		GetD3DDevice() const { return pD3DDevice; }
};

typedef Ptr<CD3D11GPUDriver> PD3D11GPUDriver;

}

#endif
