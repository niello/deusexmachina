#pragma once
#ifndef __DEM_L1_RENDER_GPU_DRIVER_H__
#define __DEM_L1_RENDER_GPU_DRIVER_H__

#include <Core/Object.h>
#include <Render/RenderFwd.h>
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

namespace Render
{
typedef Ptr<class CVertexLayout> PVertexLayout;
typedef Ptr<class CVertexBuffer> PVertexBuffer;
typedef Ptr<class CIndexBuffer> PIndexBuffer;

enum EMSAAQuality
{
	MSAA_None	= 0,
	MSAA_2x		= 2,
	MSAA_4x		= 4,
	MSAA_8x		= 8
};

class CGPUDriver: public Core::CObject
{
protected:

	//!!!see RenderSrv!

	//???to class?
	//!!!all bools to flags!
	struct CSwapChainData
	{
		POSWindow		TargetWindow;
		PDisplayDriver	TargetDisplay; //???store restricted display separately and keep this up-to-date?
		//???bool Fullscreen; or check TargetDisplay = NULL?

		//???store refresh rate or always window = 0 full = from display mode?
		//curr target display mode is always available

		DWORD			BufferCount;		// Including the front buffer
		ushort			BackBufferWidth;
		ushort			BackBufferHeight;
		EPixelFormat	BackBufferFormat;	//???or from display mode?
		EPixelFormat	DepthStencilFormat;

		EMSAAQuality	AntiAliasQuality;

		// Swap effect
		// Presentation interval (vsync or other possibilities)
			//bool			VSync;
		// Additional presentation flags

		//!!!determine how they should work and do I need them!
		//!!!to flags!
		bool			AutoAdjustSize;				// Autoadjust viewport (display mode W & H) when window size changes
		bool			DisplayModeSwitchEnabled;	// Allows to change display mode wnd->fullscr to closest to wnd size

		//???need? what about other statistics?
		DWORD			FrameID;

		//???API-specific resource pointers?
	};
	//Fullscreen(false),
	//VSync(false),
	//AutoAdjustSize(true),
	//DisplayModeSwitchEnabled(true),
	//TripleBuffering(false),
	//AntiAliasQuality(MSAA_None)

	DWORD							Adapter;

	//!!!Swap chains //!!!no direct control, manage through GPU driver! (D3D9 implicit swap chain can't be directly accessed)

	CDict<CStrID, PVertexLayout>	VertexLayouts;

	//default RT
	//render stats (frame count, fps, dips, prims etc)
	//???resource manager capabilities for VRAM resources? at least can handle OnLost-OnReset right here.

	PRenderTarget						CurrRT[MaxRenderTargetCount];
	PVertexBuffer						CurrVB[MaxVertexStreamCount];
	DWORD								CurrVBOffset[MaxVertexStreamCount];
	PVertexLayout						CurrVLayout;
	PIndexBuffer						CurrIB;
	CMeshGroup							CurrPrimGroup; //???or pointer? or don't store and pass by ref to Draw() calls?
	DWORD								InstanceCount;	// If 0, non-instanced rendering is active

	//???
	EPixelFormat						CurrDepthStencilFormat;
	IDirect3DSurface9*					pCurrDSSurface;

	//???per-swapchain? put under STATS define?
	DWORD								PrimsRendered;
	DWORD								DIPsRendered;

public:

	CGPUDriver() {}
	virtual ~CGPUDriver() { }

	bool					CheckCaps(ECaps Cap);

	//!!!Make backbuffer size match window size -> Must have window reference. Or use RequestDisplayMode() to change backbuffer?
	void					AdjustSize(/*swap chain index, 0 for default*/);

	virtual PVertexLayout	CreateVertexLayout() = 0; // Prefer GetVertexLayout() when possible
	virtual PVertexBuffer	CreateVertexBuffer() = 0;
	virtual PIndexBuffer	CreateIndexBuffer() = 0;
	PVertexLayout			GetVertexLayout(const CArray<CVertexComponent>& Components);
	PVertexLayout			GetVertexLayout(CStrID Signature) const;

	void				SetRenderTarget(DWORD Index, CRenderTarget* pRT);
	void				SetVertexLayout(CVertexLayout* pVLayout);
	void				SetVertexBuffer(DWORD Index, CVertexBuffer* pVB, DWORD OffsetVertex = 0);
	void				SetIndexBuffer(CIndexBuffer* pIB);
	void				SetInstanceBuffer(DWORD Index, CVertexBuffer* pVB, DWORD Instances, DWORD OffsetVertex = 0);
	void				SetPrimitiveGroup(const CMeshGroup& Group) { CurrPrimGroup = Group; }

	bool				BeginFrame();
	void				EndFrame();
	void				Clear(DWORD Flags, DWORD Color, float Depth, uchar Stencil);
	void				Draw();

	//!!!need swap chain index!
	void				Present();
	void				ClearScreen(DWORD Color);
	void				SaveScreenshot(EImageFormat ImageFormat /*use image codec ref?*/, IO::CStream& OutStream);
	//DWORD CreateSwapChain(swapchain params, OS window, CDisplayDriver* pFullscreenOutput = NULL)
	//bool SetFullscreen(swapchain idx, CDisplayDriver* pOutput = NULL, const CDisplayMode* pMode = NULL)
	//bool SetWindowed(swapchain idx) //???pass optional window position and size?
	DWORD				GetFrameID() const { return FrameID; }
	DWORD				GetBackBufferWidth() const { return D3DPresentParams.BackBufferWidth; }
	DWORD				GetBackBufferHeight() const { return D3DPresentParams.BackBufferHeight; }
	//???or return swap chain info struct?
	//!!!in ToggleFullscreenMode, when true, must be specified display mode. If no, system autoselects it!
};

typedef Ptr<CGPUDriver> PGPUDriver;

PVertexLayout CGPUDriver::GetVertexLayout(CStrID Signature) const
{
	int Idx = VertexLayouts.FindIndex(Signature);
	return Idx != INVALID_INDEX ? VertexLayouts.ValueAt(Idx) : NULL;
}
//---------------------------------------------------------------------

}

#endif
