#pragma once
#include <Render/DisplayDriver.h>

// DXGI display adapter driver, compatible with Direct3D 11

//???rename to DXGI[version]DisplayDriver?

struct IDXGIOutput;

namespace Render
{
typedef Ptr<class CD3D11DriverFactory> PD3D11DriverFactory;

class CD3D11DisplayDriver: public CDisplayDriver
{
	__DeclareClassNoFactory;

protected:

	friend class CD3D11DriverFactory;

	PD3D11DriverFactory _DriverFactory;
	IDXGIOutput*        pDXGIOutput = nullptr;

	CD3D11DisplayDriver(CD3D11DriverFactory& DriverFactory);

	virtual bool Init(UPTR AdapterNumber, UPTR OutputNumber);
	virtual void Term() { InternalTerm(); CDisplayDriver::Term(); } //???need? or never manually-destructible?
	void         InternalTerm();

public:

	virtual ~CD3D11DisplayDriver() { InternalTerm(); }

	virtual UPTR GetAvailableDisplayModes(EPixelFormat Format, std::vector<CDisplayMode>& OutModes) const;
	virtual bool SupportsDisplayMode(const CDisplayMode& Mode) const;
	virtual bool GetCurrentDisplayMode(CDisplayMode& OutMode) const;
	virtual bool GetDisplayMonitorInfo(CMonitorInfo& OutInfo) const;
	IDXGIOutput* GetDXGIOutput() const { return pDXGIOutput; }
};

typedef Ptr<CD3D11DisplayDriver> PD3D11DisplayDriver;

}
