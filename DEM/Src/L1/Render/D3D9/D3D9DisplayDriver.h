#pragma once
#ifndef __DEM_L1_RENDER_D3D9_DISPLAY_DRIVER_H__
#define __DEM_L1_RENDER_D3D9_DISPLAY_DRIVER_H__

#include <Render/DisplayDriver.h>

// Direct3D9 display adapter driver

typedef enum _D3DFORMAT D3DFORMAT;

namespace Render
{

class CD3D9DisplayDriver: public CDisplayDriver
{
	__DeclareClass(CD3D9DisplayDriver);

protected:

	//???cache display modes?
	//!!!device name can be get from adapter!

public:

	CD3D9DisplayDriver() {}
	virtual ~CD3D9DisplayDriver() { }

	static D3DFORMAT	PixelFormatToD3DFormat(EPixelFormat Format);
	static EPixelFormat	D3DFormatToPixelFormat(D3DFORMAT D3DFormat);

	virtual void		GetAvailableDisplayModes(EPixelFormat Format, CArray<CDisplayMode>& OutModes) const;
	virtual bool		SupportsDisplayMode(const CDisplayMode& Mode) const;
	virtual bool		GetCurrentDisplayMode(CDisplayMode& OutMode) const;
	virtual void		GetMonitorInfo(CMonitorInfo& OutInfo) const;
};

}

#endif
