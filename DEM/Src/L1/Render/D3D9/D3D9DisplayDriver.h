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

public:

	CD3D9DisplayDriver() {}
	virtual ~CD3D9DisplayDriver() { }
};

}

#endif
