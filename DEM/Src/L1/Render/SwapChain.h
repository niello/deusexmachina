#pragma once
#ifndef __DEM_L1_RENDER_SWAP_CHAIN_H__
#define __DEM_L1_RENDER_SWAP_CHAIN_H__

//#include <Render/DisplayMode.h>
#include <Render/RenderFwd.h>
#include <Data/Flags.h>
#include <Data/Ptr.h>

// Swap chain is a set of a front buffer and one or more back buffers. GPU device renders into a
// back buffer and presents it to a display (fullscreen) or a desktop (windowed). Presented back
// buffer becomes a front buffer, and a former front buffer becomes the last back buffer, and so on.
// This header contains definitions for a swap chain description, as long as swap chain classes.

namespace Sys
{
	typedef Ptr<class COSWindow> POSWindow; //???need to make window RefCounted?
}

namespace Render
{
typedef Ptr<class CDisplayDriver> PDisplayDriver;

enum ESwapChainFlags
{
	//ManualRotationHandling
	//AutoSwitchModeOnResizeTarget // switches on resize target and on wnd->fullscr //???always handle by app?
	//AutoAdjustSize // resize buffers to window / display mode
	//FrameLatencyWaitable // DXGI since Win8.1
	SwapChain_VSync //???need other intervals, like 2-4 (each 2nd-4th vsync)? if need, move to separate enum or use integer!
};

enum ESwapMode
{
	SwapMode_CopyDiscard,	// Copy back buffer to front buffer, discard back buffer contents
	SwapMode_CopyPersist,	// Copy back buffer to front buffer, persist back buffer contents
	SwapMode_FlipPersist	// Flip buffers, persist back buffer contents
};

struct CSwapChainDesc
{
	Data::CFlags	Flags;
	ESwapMode		SwapMode;

	DWORD			BufferCount;		// Including the front buffer
	//CDisplayMode	BufferMode;			//???are all members needed?  DXGI uses full display mode for a swap chain! fullscreen only?
	ushort			BackBufferWidth;	// Set to 0 to match window or display format
	ushort			BackBufferHeight;	// Set to 0 to match window or display format
	EPixelFormat	BackBufferFormat;	//???always get from passed display format?

	EPixelFormat	DepthStencilFormat;

	EMSAAQuality	AntiAliasQuality;

	//other flags, presentation and not (autoadjustsize - match backbuffer size to target size, or always?)

	CSwapChainDesc(): AntiAliasQuality(MSAA_None) {}
};

// Inherit from this class to create API-specific implementations
class CSwapChainBase
{
public:

	Sys::POSWindow	TargetWindow;	//???to desc?
	PDisplayDriver	TargetDisplay;
	CSwapChainDesc	Desc;

	//???need? what about other statistics?
	DWORD			FrameID;

	bool IsFullscreen() const { return TargetDisplay.IsValid(); }
};

}

#endif
