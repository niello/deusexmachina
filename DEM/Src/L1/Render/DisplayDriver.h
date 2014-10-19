#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_DRIVER_H__
#define __DEM_L1_RENDER_DISPLAY_DRIVER_H__

#include <Core/Object.h>
#include <Render/DisplayMode.h>

// Display adapter driver represents and provides interface to manipulate with a display device.
// Display device is an output of some video adapter, typically a monitor. Create display
// drivers with CVideoDriverFactory.
// You can implement this class via some graphics API, like D3D9 or DXGI.

//???1 swap-chain + multiple outputs?
// Fullscreen: SwapChain -> DisplayMode -> Output
// Windowed: SwapChain -> Window rect -> Output(s)

namespace Render
{

class CDisplayDriver: public Core::CObject
{
public:

	enum EMSAAQuality
	{
		MSAA_None	= 0,
		MSAA_2x		= 2,
		MSAA_4x		= 4,
		MSAA_8x		= 8
	};

	//???cache or get on demand?
	struct CMonitorInfo
	{
		short	Left;
		short	Top;
		ushort	Width;
		ushort	Height;
		bool	IsPrimary;
		//bool	IsAttachedToDesktop; // D3D9: HMONITOR -> DISPLAY_DEVICE
		//Data::CSimpleString DeviceName; // D3D9 - from adapter info
		//work area, monitor area in desktop coords (DPI-dependent)
		//rotation (unspec, 0, 90, 180, 270)
	};

protected:

	DWORD			Adapter;
	DWORD			Output;

	// CMonitorInfo MonitorInfo; //!!!Cache, if it makes sense

	// We don't cache available display modes since they can change after display driver was created
	CDisplayMode	CurrentMode;	// Real mode display operates in
	CDisplayMode	RequestedMode;	// Mode requested by user
	//???flag "need to update mode"? marks requested mode as dirty and reapply it to display!

	// Swap chain and window:

	// Backbuffer info (w, h, fmt, count)
	// DepthStencil fmt
	// Multisample info (type / sample count, quality)
	// Swap effect
	// Windowed/Fullscreen
	//???refresh rate? 0 for windowed, from disp mode for fullscreen (at least in D3D9) // So needn't to store here.
	// Presentation interval - relation between adapter refresh rate and Present() call rate
	// Different additional Present() options

	// Device window
	// Alt-Enter focus window //???D3D9-only?
	// D3D9: For fullscreen, one adapter can use focus window as device window, others should use other device windows

	EMSAAQuality	AntiAliasQuality;

	//!!!to flags!
	bool			Fullscreen;
	bool			VSync;
	bool			AutoAdjustSize;				// Autoadjust viewport (display mode W & H) when window size changes
	bool			DisplayModeSwitchEnabled;	// Allows to change display mode wnd->fullscr to closest to wnd size
	bool			TripleBuffering;			// Use double or triple buffering when fullscreen

public:

	CDisplayDriver();
	virtual ~CDisplayDriver() { }

	//!!!Make backbuffer size match window size -> Must have window reference. Or use RequestDisplayMode() to change backbuffer?
	void					AdjustSize();

	virtual void			GetAvailableDisplayModes(EPixelFormat Format, CArray<CDisplayMode>& OutModes) const = 0;
	virtual bool			SupportsDisplayMode(const CDisplayMode& Mode) const = 0;
	virtual bool			GetCurrentDisplayMode(CDisplayMode& OutMode) const = 0;
	virtual void			GetMonitorInfo(CMonitorInfo& OutInfo) const = 0;

	void					GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const;
	void					GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	void					RequestDisplayMode(const CDisplayMode& Mode) { RequestedMode = Mode; }
	const CDisplayMode&		GetDisplayMode() const { return DisplayMode; }
	const CDisplayMode&		GetRequestedDisplayMode() const { return RequestedMode; }
};

typedef Ptr<CDisplayDriver> PDisplayDriver;

inline CDisplayDriver::CDisplayDriver(DWORD AdapterNumber, DWORD OutputNumber):
	Adapter(AdapterNumber),
	Output(OutputNumber),
	Fullscreen(false),
	VSync(false),
	AutoAdjustSize(true),
	DisplayModeSwitchEnabled(true),
	TripleBuffering(false),
	AntiAliasQuality(MSAA_None)
{
}
//---------------------------------------------------------------------

inline void CDisplayDriver::GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const
{
	XAbs = (int)(XRel * CurrentMode.Width);
	YAbs = (int)(YRel * CurrentMode.Height);
}
//---------------------------------------------------------------------

inline void CDisplayDriver::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	XRel = XAbs / float(CurrentMode.Width);
	YRel = YAbs / float(CurrentMode.Height);
}
//---------------------------------------------------------------------

}

#endif
