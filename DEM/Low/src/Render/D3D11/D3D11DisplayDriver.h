#pragma once
#ifndef __DEM_L1_RENDER_D3D11_DISPLAY_DRIVER_H__
#define __DEM_L1_RENDER_D3D11_DISPLAY_DRIVER_H__

#include <Render/DisplayDriver.h>

// DXGI display adapter driver, compatible with Direct3D 11

//???rename to DXGI[version]DisplayDriver?

struct IDXGIOutput;

namespace Render
{

class CD3D11DisplayDriver: public CDisplayDriver
{
	__DeclareClass(CD3D11DisplayDriver);

protected:

	friend class CD3D11DriverFactory;

	IDXGIOutput*	pDXGIOutput;

	CD3D11DisplayDriver(): pDXGIOutput(NULL) {}

	virtual bool	Init(UPTR AdapterNumber, UPTR OutputNumber);
	virtual void	Term() { InternalTerm(); CDisplayDriver::Term(); } //???need? or never manually-destructible?
	void			InternalTerm();

public:

	virtual ~CD3D11DisplayDriver() { InternalTerm(); }

	virtual UPTR	GetAvailableDisplayModes(EPixelFormat Format, CArray<CDisplayMode>& OutModes) const;
	virtual bool	SupportsDisplayMode(const CDisplayMode& Mode) const;
	virtual bool	GetCurrentDisplayMode(CDisplayMode& OutMode) const;
	virtual bool	GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const;
	IDXGIOutput*	GetDXGIOutput() const { return pDXGIOutput; }
};

typedef Ptr<CD3D11DisplayDriver> PD3D11DisplayDriver;

}

#endif
