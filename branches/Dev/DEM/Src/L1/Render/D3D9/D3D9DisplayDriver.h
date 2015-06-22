#pragma once
#ifndef __DEM_L1_RENDER_D3D9_DISPLAY_DRIVER_H__
#define __DEM_L1_RENDER_D3D9_DISPLAY_DRIVER_H__

#include <Render/DisplayDriver.h>

// Direct3D9 display adapter driver

namespace Render
{

class CD3D9DisplayDriver: public CDisplayDriver
{
	__DeclareClass(CD3D9DisplayDriver);

protected:

	friend class CD3D9DriverFactory;

	CD3D9DisplayDriver() {}

	virtual bool	Init(DWORD AdapterNumber, DWORD OutputNumber);
	//virtual void	Term() { InternalTerm(); CDisplayDriver::Term(); } //???need? or never manually-destructible?
	//void			InternalTerm();

public:

	//virtual ~CD3D9DisplayDriver() { InternalTerm(); }

	virtual DWORD	GetAvailableDisplayModes(EPixelFormat Format, CArray<CDisplayMode>& OutModes) const;
	virtual bool	SupportsDisplayMode(const CDisplayMode& Mode) const;
	virtual bool	GetCurrentDisplayMode(CDisplayMode& OutMode) const;
	virtual bool	GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const;
};

typedef Ptr<CD3D9DisplayDriver> PD3D9DisplayDriver;

}

#endif
