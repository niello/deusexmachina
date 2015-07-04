#pragma once
#ifndef __DEM_L1_CEGUI_DEM_FWD_H__
#define __DEM_L1_CEGUI_DEM_FWD_H__

#include <StdDEM.h>

// Forward declarations for DeusExMachina CEGUI implementation

namespace CEGUI
{

struct D3DVertex
{
	float x, y, z; // Transformed
	DWORD diffuse;
	float tu, tv;
};

}

#endif