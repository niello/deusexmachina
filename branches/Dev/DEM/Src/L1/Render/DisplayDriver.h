#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_DRIVER_H__
#define __DEM_L1_RENDER_DISPLAY_DRIVER_H__

#include <Core/Object.h>
#include <Render/DisplayMode.h>

// Display adapter driver represents and provides interface to manipulate with a display device.
// Display device is an output of some video adapter, typically a monitor.
// Create display drivers with CVideoDriverFactory.
// You can implement this class via some graphics API, like D3D9 or DXGI.

namespace Render
{

class CDisplayDriver: public Core::CObject
{
public:

	//???cache? store right here?
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

public:

	CDisplayDriver(): Adapter(Adapter_None), Output(Output_None) {}
	virtual ~CDisplayDriver() {}

	virtual bool			Init(DWORD AdapterNumber, DWORD OutputNumber) { Adapter = AdapterNumber; Output = OutputNumber; OK; }
	virtual void			Term() { Adapter = Adapter_None; Output = Output_None; }

	virtual DWORD			GetAvailableDisplayModes(EPixelFormat Format, CArray<CDisplayMode>& OutModes) const = 0;
	virtual bool			SupportsDisplayMode(const CDisplayMode& Mode) const = 0;
	virtual bool			GetCurrentDisplayMode(CDisplayMode& OutMode) const = 0;
	virtual bool			GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const = 0;
	//???find closest display mode? may be non-virtual with my own algorithm, but DXGI has its own implementation

	DWORD					GetAdapterID() const {return Adapter; }
	DWORD					GetAdapterOutputID() const {return Output; }
	bool					IsInitialized() const { return Adapter != Adapter_None; }
};

typedef Ptr<CDisplayDriver> PDisplayDriver;

}

#endif
