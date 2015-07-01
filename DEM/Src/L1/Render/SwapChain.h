#pragma once
#ifndef __DEM_L1_RENDER_SWAP_CHAIN_H__
#define __DEM_L1_RENDER_SWAP_CHAIN_H__

#include <Data/Ptr.h>
#include <Data/Flags.h>
#include <Data/Regions.h>

// Swap chain is a set of a front buffer and one or more back buffers. GPU device renders into a
// back buffer and presents it to a display (fullscreen) or a desktop (windowed). Presented back
// buffer becomes a front buffer, and a former front buffer becomes the last back buffer, and so on.
// Inherit from CSwapChain to create API-specific implementationsþ

namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow;
}

namespace Render
{
typedef Ptr<class CDisplayDriver> PDisplayDriver;
typedef Ptr<class CRenderTarget> PRenderTarget;

//!!!other flags, presentation and not
// No VSync when windowed can cause jerking at low frame rates
enum ESwapChainFlags
{
	//ManualRotationHandling
	//AutoSwitchModeOnResizeTarget // switches on resize target and on wnd->fullscr //???always handle by app?
	//FrameLatencyWaitable // DXGI since Win8.1
	SwapChain_AutoAdjustSize	= 0x01,	// Listens OS window resize event and resizes buffers accordingly
	SwapChain_VSync				= 0x02	//???need other intervals, like 2-4 (each 2nd-4th vsync)? if need, move to separate enum or use integer!
};

enum ESwapMode
{
	SwapMode_CopyDiscard,	// Copy back buffer to front buffer, discard back buffer contents
	SwapMode_CopyPersist,	// Copy back buffer to front buffer, persist back buffer contents
	SwapMode_FlipPersist	// Flip buffers, persist back buffer contents
};

// Buffer size, format and MSAA is described with CRenderTargetDesc
struct CSwapChainDesc
{
	Data::CFlags		Flags;
	ESwapMode			SwapMode;
	DWORD				BackBufferCount;

	CSwapChainDesc(): Flags(SwapChain_AutoAdjustSize | SwapChain_VSync) {}
};

class CSwapChain
{
public:

	PRenderTarget	BackBufferRT;
	Sys::POSWindow	TargetWindow;	//???to desc?
	Data::CRect		LastWindowRect;	// Stores a window size in a windowed mode
	PDisplayDriver	TargetDisplay;
	//CDisplayMode	DisplayMode;	//???store as (p)LastDisplayMode? to preserve selected mode when full -> wnd -> full
	CSwapChainDesc	Desc;

	//???need? what about other statistics?
	DWORD			FrameID;

	CSwapChain(): FrameID(0) {}

	bool IsValid() const { return TargetWindow.IsValidPtr(); }
	bool IsFullscreen() const { return TargetDisplay.IsValidPtr(); }
};

}

#endif
