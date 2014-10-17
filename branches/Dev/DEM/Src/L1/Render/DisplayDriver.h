#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_DRIVER_H__
#define __DEM_L1_RENDER_DISPLAY_DRIVER_H__

#include <Core/Object.h>
#include <Render/DisplayMode.h>

// Display adapter driver represents and provides interface to manipulate with a display,
// including its mode, swap chain, refresh rate, buffer formats etc.
// Implementations of this class are typically based on some graphics API, like D3D9 or DXGI.

//!!!what about connecting GPU to display adapter, display adapter to OS window etc?
//???!!!singleton? or one per active display adapter? how multi-monitor/multi-viewport works?

namespace Render
{

class CDisplayDriver: public Core::CObject
{
public:

	//???UINT and defines for primary & secondary instead of enum?
	enum EAdapter
	{
		Adapter_None = -1,
		Adapter_Primary = 0,
		Adapter_Secondary = 1
	};

	enum EMSAAQuality
	{
		MSAA_None	= 0,
		MSAA_2x		= 2,
		MSAA_4x		= 4,
		MSAA_8x		= 8
	};

	struct CMonitorInfo
	{
		ushort	Left;
		ushort	Top;
		ushort	Width;
		ushort	Height;
		bool	IsPrimary;
	};

protected:

	CDisplayMode	DisplayMode;
	CDisplayMode	RequestedMode;
	EMSAAQuality	AntiAliasQuality;

	EAdapter		Adapter;

	//!!!to flags!
	bool			Fullscreen;
	bool			VSync;
	bool			AutoAdjustSize;				// Autoadjust viewport (display mode W & H) when window size changes
	bool			DisplayModeSwitchEnabled;	//???
	bool			TripleBuffering;			// Use double or triple buffering when fullscreen

	//!!!see RenderSrv, Display!

	//adapter info (ID, prim/sec, vendor, device etc)
	//mode, msaa, swapeffect, freq hz, fullscreen, vsync, back buffer count and settings
	//   (see D3DPRESENT_PARAMETERS)
	//requested display mode
	//static methods for adapter manipulation
	//allow switch, autoadjust size
	//???swap chain?

public:

	CDisplayDriver() {}
	virtual ~CDisplayDriver() { }

	void				AdjustSize();

	bool				AdapterExists(EAdapter Adapter);
	void				GetAvailableDisplayModes(EAdapter Adapter, EPixelFormat Format, CArray<CDisplayMode>& OutModes);
	bool				SupportsDisplayMode(EAdapter Adapter, const CDisplayMode& Mode);
	bool				GetCurrentAdapterDisplayMode(EAdapter Adapter, CDisplayMode& OutMode);
	//CAdapterInfo		GetAdapterInfo(EAdapter Adapter);
	void				GetAdapterMonitorInfo(EAdapter Adapter, CMonitorInfo& OutInfo);

	// Based on back buffer size
	void				GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const;
	void				GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	void				RequestDisplayMode(const CDisplayMode& Mode) { RequestedMode = Mode; }
	const CDisplayMode&	GetDisplayMode() const { return DisplayMode; }
	const CDisplayMode&	GetRequestedDisplayMode() const { return RequestedMode; }
};

inline CDisplayDriver::CDisplayDriver():
	Fullscreen(false),
	VSync(false),
	AutoAdjustSize(true),
	DisplayModeSwitchEnabled(true),
	TripleBuffering(false),
	Adapter(Adapter_Primary),
	AntiAliasQuality(MSAA_None)
{
}
//---------------------------------------------------------------------

inline void CDisplayDriver::GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const
{
	XAbs = (int)(XRel * DisplayMode.Width);
	YAbs = (int)(YRel * DisplayMode.Height);
}
//---------------------------------------------------------------------

inline void CDisplayDriver::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	XRel = XAbs / float(DisplayMode.Width);
	YRel = YAbs / float(DisplayMode.Height);
}
//---------------------------------------------------------------------

}

#endif
