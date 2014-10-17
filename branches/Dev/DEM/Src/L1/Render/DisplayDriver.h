#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_DRIVER_H__
#define __DEM_L1_RENDER_DISPLAY_DRIVER_H__

#include <Core/Object.h>

// Display adapter driver represents and provides interface to manipulate with a display,
// including its mode, swap chain, refresh rate, buffer formats etc.
// Implementations of this class are typically based on some graphics API, like D3D9 or DXGI.

//!!!what about connecting GPU to display adapter, display adapter to OS window etc?
//???!!!singleton? or one per active display adapter? how multi-monitor/multi-viewport works?

namespace Render
{

class CDisplayDriver: public Core::CObject
{
protected:

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
};

}

#endif
