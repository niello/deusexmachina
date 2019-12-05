#pragma once
#include <Render/DisplayDriver.h>

// Direct3D9 display adapter driver

namespace Render
{
typedef Ptr<class CD3D9DriverFactory> PD3D9DriverFactory;

class CD3D9DisplayDriver: public CDisplayDriver
{
	RTTI_CLASS_DECL;

protected:

	friend class CD3D9DriverFactory;

	PD3D9DriverFactory _DriverFactory;

	CD3D9DisplayDriver(CD3D9DriverFactory& DriverFactory);

	virtual bool	Init(UPTR AdapterNumber, UPTR OutputNumber);
	//virtual void	Term() { InternalTerm(); CDisplayDriver::Term(); } //???need? or never manually-destructible?
	//void			InternalTerm();

public:

	//virtual ~CD3D9DisplayDriver() { InternalTerm(); }

	virtual UPTR	GetAvailableDisplayModes(EPixelFormat Format, std::vector<CDisplayMode>& OutModes) const;
	virtual bool	SupportsDisplayMode(const CDisplayMode& Mode) const;
	virtual bool	GetCurrentDisplayMode(CDisplayMode& OutMode) const;
	virtual bool	GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const;
};

typedef Ptr<CD3D9DisplayDriver> PD3D9DisplayDriver;

}
