#pragma once
#include <Core/Object.h>
#include <Data/Array.h>
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
		I16		Left = 0;
		I16		Top = 0;
		U16		Width = 0;
		U16		Height = 0;
		U16		NativeWidth = 0;
		U16		NativeHeight = 0;
		bool	IsPrimary = false;
		//bool	IsAttachedToDesktop; // D3D9: HMONITOR -> DISPLAY_DEVICE
		//CString DeviceName; // D3D9 - from adapter info
		//work area, monitor area in desktop coords (DPI-dependent)
		//rotation (unspec, 0, 90, 180, 270)
	};

protected:

	UPTR			AdapterID;
	UPTR			OutputID;

public:

	CDisplayDriver(): AdapterID(Adapter_None), OutputID(Output_None) {}
	virtual ~CDisplayDriver() {}

	virtual bool			Init(UPTR AdapterNumber, UPTR OutputNumber) { AdapterID = AdapterNumber; OutputID = OutputNumber; OK; }
	virtual void			Term() { AdapterID = Adapter_None; OutputID = Output_None; }

	virtual UPTR			GetAvailableDisplayModes(EPixelFormat Format, std::vector<CDisplayMode>& OutModes) const = 0;
	virtual bool			SupportsDisplayMode(const CDisplayMode& Mode) const = 0;
	virtual bool			GetCurrentDisplayMode(CDisplayMode& OutMode) const = 0;
	virtual bool			GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const = 0;
	//???find closest display mode? may be non-virtual with my own algorithm, but DXGI has its own implementation

	UPTR					GetAdapterID() const {return AdapterID; }
	UPTR					GetAdapterOutputID() const {return OutputID; }
	bool					IsInitialized() const { return AdapterID != Adapter_None; }
};

typedef Ptr<CDisplayDriver> PDisplayDriver;

}
