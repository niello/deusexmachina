#pragma once
#ifndef __DEM_L1_RENDER_GPU_DRIVER_H__
#define __DEM_L1_RENDER_GPU_DRIVER_H__

#include <Core/Object.h>
#include <Render/SwapChain.h>
#include <Render/VertexComponent.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>

// GPU device driver manages VRAM resources and provides an interface for rendering on a video card.
// Create GPU device drivers with CVideoDriverFactory.
// You can implement this class via some graphics API, like D3D or OpenGL.
// Each GPU device can manage zero or more swap chains, either windowed or fullscreen. Each fullscreen
// swap chain must be connected to a corresponding display output, represented by a CDisplayDriver instance.
// Windowed swap chains are present to the desktop. Each swap chain requires a viewport, and now
// I use COSWindow for it.

//!!!each adapter output can display only one fullscreen swap chain! control it!
//D3D9: multihead (multi-swap-chain) device must have all swap chains fullscreen
//???can have multiple windowed swap chains?
//when going from windowed to fullscreen, use display driver of monitor that contains majority of window area
//also display driver should match display mode etc, for windowed can use D3DFMT_UNKNOWN and window size (or 0x0 for auto)
//IDirect3DSwapChain9::GetDisplayMode() for multihead device
//focus window must be shared among all swap chains, and at least in D3D9 - among all devices in application
// also focus window should be a parent of any device winow, or be itself.
// since so, D3D9DriverFactory may contain this window's reference
//!!!GPU driver can be used without display, for stream-out, render to texture or compute shaders!

namespace Data
{
	class CParams;
}

namespace Render
{
class CDisplayMode;
typedef Ptr<class CVertexLayout> PVertexLayout;
typedef Ptr<class CVertexBuffer> PVertexBuffer;
typedef Ptr<class CIndexBuffer> PIndexBuffer;
typedef Ptr<class CRenderState> PRenderState;

class CGPUDriver: public Core::CObject
{
public:

	//!!!to variables!
	//enum
	//{
	//	MaxTextureStageCount = 8, //???16?
	//	MaxRenderTargetCount = 4,
	//	MaxVertexStreamCount = 2 // Not sure why 2, N3 value
	//};

protected:

	DWORD							Adapter;

	CDict<CStrID, PVertexLayout>	VertexLayouts;

	//default RT
	//???resource manager capabilities for VRAM resources? at least can handle OnLost-OnReset right here.

	/*
	PRenderTarget					CurrRT[MaxRenderTargetCount];
	PVertexBuffer					CurrVB[MaxVertexStreamCount];
	DWORD							CurrVBOffset[MaxVertexStreamCount];
	PVertexLayout					CurrVLayout;
	PIndexBuffer					CurrIB;
	CMeshGroup						CurrPrimGroup; //???or pointer? or don't store and pass by ref to Draw() calls?
	DWORD							InstanceCount;	// If 0, non-instanced rendering is active
	*/

	//???
	//!!!per-swapchain!? WHERE IS USED?
	/*
	EPixelFormat					CurrDepthStencilFormat;
	IDirect3DSurface9*				pCurrDSSurface;
	*/

	//???per-swapchain? put under STATS define?
	DWORD							PrimsRendered;
	DWORD							DIPsRendered;
	
	virtual PVertexLayout	InternalCreateVertexLayout() = 0;

public:

	CGPUDriver() {}
	virtual ~CGPUDriver() {}

	virtual bool			Init(DWORD AdapterNumber) { Adapter = AdapterNumber; OK; }
	virtual bool			CheckCaps(ECaps Cap) = 0;

	virtual DWORD			CreateSwapChain(const CSwapChainDesc& Desc, Sys::COSWindow* pWindow) = 0;
	virtual bool			DestroySwapChain(DWORD SwapChainID) = 0;
	virtual bool			SwapChainExists(DWORD SwapChainID) const = 0;
	virtual bool			SwitchToFullscreen(DWORD SwapChainID, const CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL) = 0;
	virtual bool			SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL) = 0;
	virtual bool			ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height);
	virtual bool			IsFullscreen(DWORD SwapChainID) const = 0;
	//!!!get info, change info (or only recreate?)
	virtual bool			Present(DWORD SwapChainID) = 0;
	bool					PresentBlankScreen(DWORD SwapChainID, DWORD Color);
	//virtual void			SaveScreenshot(DWORD SwapChainID, EImageFormat ImageFormat /*use image codec ref?*/, IO::CStream& OutStream) = 0;

	bool					BeginFrame();
	void					EndFrame();
	void					Clear(DWORD Flags, DWORD Color, float Depth, uchar Stencil);
	void					Draw();

	virtual PVertexBuffer	CreateVertexBuffer() = 0;
	virtual PIndexBuffer	CreateIndexBuffer() = 0;
	PVertexLayout			CreateVertexLayout(const CArray<CVertexComponent>& Components);
	PVertexLayout			GetVertexLayout(CStrID Signature) const;
	virtual PRenderState	CreateRenderState(const Data::CParams& Desc) = 0;

	//void					SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	void					SetVertexLayout(CVertexLayout* pVLayout);
	void					SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex = 0);
	void					SetIndexBuffer(CIndexBuffer* pIB);
	void					SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex = 0);
	//void					SetPrimitiveGroup(const CMeshGroup& Group) { CurrPrimGroup = Group; }

	//!!!need swap chain index!
	//DWORD					GetFrameID() const { return FrameID; }
	//DWORD					GetBackBufferWidth() const { return D3DPresentParams.BackBufferWidth; }
	//DWORD					GetBackBufferHeight() const { return D3DPresentParams.BackBufferHeight; }
	//???or return swap chain info struct?
};

typedef Ptr<CGPUDriver> PGPUDriver;

inline PVertexLayout CGPUDriver::GetVertexLayout(CStrID Signature) const
{
	int Idx = VertexLayouts.FindIndex(Signature);
	return Idx != INVALID_INDEX ? VertexLayouts.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

inline bool CGPUDriver::PresentBlankScreen(DWORD SwapChainID, DWORD Color)
{
	//???set swap chain render target? or pass id to beginframe and set inside?
	//internal check must be performed not to reset target already set
	if (BeginFrame())
	{
		Clear(Clear_Color, Color, 1.f, 0); //???clear depth and stencil too?
		EndFrame();
		Present(SwapChainID);
	}
}
//---------------------------------------------------------------------

}

#endif
