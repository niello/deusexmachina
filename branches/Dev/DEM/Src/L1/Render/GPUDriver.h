#pragma once
#ifndef __DEM_L1_RENDER_GPU_DRIVER_H__
#define __DEM_L1_RENDER_GPU_DRIVER_H__

#include <Core/Object.h>
//#include <Render/RenderTarget.h>
//#include <Render/SwapChain.h>
#include <Render/RenderFwd.h>
#include <Render/VertexComponent.h>
#include <Data/Dictionary.h>
#include <Data/StringID.h>
#include <Data/Rect.h>

// GPU device driver manages VRAM resources and provides an interface for rendering on a video card.
// Create GPU device drivers with CVideoDriverFactory.
// You can implement this class via some graphics API, like D3D or OpenGL.
// Each GPU device can manage zero or more swap chains, either windowed or fullscreen. Each fullscreen
// swap chain must be connected to a corresponding display output, represented by a CDisplayDriver instance.
// Windowed swap chains are present to the desktop. Each swap chain requires a viewport, and now
// I use COSWindow for it.

//PERF:
//!!!GCN says load shaders before textures, driver compiles to its ASM in the background!
//then warm shader cache - bind all shaders and perform offscreen rendering

//!!!each adapter output can display only one fullscreen swap chain! control it!
//D3D9: multihead (multi-swap-chain) device must have all swap chains fullscreen
//???can have multiple windowed swap chains?
//when going from windowed to fullscreen, use display driver of monitor that contains majority of window area
//also display driver should match display mode etc, for windowed can use D3DFMT_UNKNOWN and window size (or 0x0 for auto)
//IDirect3DSwapChain9::GetDisplayMode() for multihead device
//focus window must be shared among all swap chains, and at least in D3D9 - among all devices in application
// also focus window should be a parent of any device window, or be itself.
// since so, D3D9DriverFactory may contain this window's reference
//!!!GPU driver can be used without display, for stream-out, render to texture or compute shaders!

namespace Data
{
	class CParams;
}

namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}

namespace Render
{
struct CRenderTargetDesc;
struct CSwapChainDesc;
class CDisplayMode;
typedef Ptr<class CDisplayDriver> PDisplayDriver;
typedef Ptr<class CRenderTarget> PRenderTarget;
typedef Ptr<class CVertexLayout> PVertexLayout;
typedef Ptr<class CVertexBuffer> PVertexBuffer;
typedef Ptr<class CIndexBuffer> PIndexBuffer;
typedef Ptr<class CDepthStencilBuffer> PDepthStencilBuffer;
typedef Ptr<class CRenderState> PRenderState;
typedef Ptr<class CShader> PShader;

class CGPUDriver: public Core::CObject
{
protected:

	DWORD							AdapterID;
	EGPUDriverType					Type;

	CDict<CStrID, PVertexLayout>	VertexLayouts;

	//default RT
	//???resource manager capabilities for VRAM resources? at least can handle OnLost-OnReset right here.

	/*
	//!!!to variables (caps)!
	//enum
	//{
	//	MaxTextureStageCount = 8, //???16?
	//	MaxRenderTargetCount = 4,
	//	MaxVertexStreamCount = 2 // Not sure why 2, N3 value
	//};
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

	static void					PrepareWindowAndBackBufferSize(Sys::COSWindow& Window, UINT& Width, UINT& Height);
	//virtual PVertexLayout	InternalCreateVertexLayout() = 0;
	//virtual HShaderParam	CreateShaderVarHandle(const CShaderConstantDesc& Meta) const = 0;

public:

	CGPUDriver() {}
	virtual ~CGPUDriver() {}

	virtual bool				Init(DWORD AdapterNumber, EGPUDriverType DriverType) { AdapterID = AdapterNumber; OK; }
	virtual bool				CheckCaps(ECaps Cap) = 0;

	virtual int					CreateSwapChain(const CRenderTargetDesc& BackBufferDesc, const CSwapChainDesc& SwapChainDesc, Sys::COSWindow* pWindow) = 0;
	virtual bool				DestroySwapChain(DWORD SwapChainID) = 0;
	virtual bool				SwapChainExists(DWORD SwapChainID) const = 0;
	virtual bool				SwitchToFullscreen(DWORD SwapChainID, CDisplayDriver* pDisplay = NULL, const CDisplayMode* pMode = NULL) = 0;
	virtual bool				SwitchToWindowed(DWORD SwapChainID, const Data::CRect* pWindowRect = NULL) = 0;
	virtual bool				ResizeSwapChain(DWORD SwapChainID, unsigned int Width, unsigned int Height) = 0;
	virtual bool				IsFullscreen(DWORD SwapChainID) const = 0;
	virtual PRenderTarget		GetSwapChainRenderTarget(DWORD SwapChainID) const = 0;
	// GetSwapChainDesc(), GetBackBufferDesc()
	//!!!get info, change info (or only recreate?)
	virtual bool				Present(DWORD SwapChainID) = 0;
	bool						PresentBlankScreen(DWORD SwapChainID, DWORD Color);
	//virtual void				SaveScreenshot(DWORD SwapChainID, EImageFormat ImageFormat /*use image codec ref?*/, IO::CStream& OutStream) = 0;

	virtual bool				BeginFrame() = 0;
	virtual void				EndFrame() = 0;
	//???what if clear float RT?
	virtual DWORD				GetMaxMultipleRenderTargetCount() = 0;
	virtual bool				SetRenderTarget(DWORD Index, CRenderTarget* RT) = 0;
	virtual void				Clear(DWORD Flags, DWORD Color, float Depth, uchar Stencil) = 0;

	virtual PVertexBuffer		CreateVertexBuffer() = 0;
	virtual PIndexBuffer		CreateIndexBuffer() = 0;
	PVertexLayout				CreateVertexLayout(const CArray<CVertexComponent>& Components /*, CStrID ShaderInputSignature = CStrID::Empty*/);
	PVertexLayout				GetVertexLayout(CStrID Signature /*, CStrID ShaderInputSignature = CStrID::Empty*/) const;
	//virtual PRenderState		CreateRenderState(const Data::CParams& Desc) = 0;
	PShader						CreateShader(const Data::CParams& Desc);
	//virtual PConstantBuffer		CreateConstantBuffer(const CShaderConstantDesc& Meta) = 0;
	//virtual PTexture				CreateTexture(dimensions, array size, format etc) = 0;
	virtual PRenderTarget		CreateRenderTarget(const CRenderTargetDesc& Desc) = 0;
	//!!!another desc struct if can't use as shader input!
	//!!!can describe as DepthBits & StencilBits, find closest on creation!
	virtual PDepthStencilBuffer	CreateDepthStencilBuffer(const CRenderTargetDesc& Desc) = 0;

	//void						SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	void						SetVertexLayout(CVertexLayout* pVLayout);
	void						SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex = 0);
	void						SetIndexBuffer(CIndexBuffer* pIB);
	void						SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex = 0);
	//void						SetPrimitiveGroup(const CMeshGroup& Group) { CurrPrimGroup = Group; }

	EGPUDriverType				GetType() const { return Type; }
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
		OK;
	}
	FAIL;
}
//---------------------------------------------------------------------

}

#endif
