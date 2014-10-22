#pragma once
#ifndef __DEM_L1_RENDER_H__
#define __DEM_L1_RENDER_H__

#include <StdDEM.h>

// Render system definitions and forward declarations

namespace Render
{
const DWORD Adapter_None = (DWORD)-1;
const DWORD Adapter_Primary = 0;
const DWORD Adapter_Secondary = 1;
const DWORD Output_None = (DWORD)-1;

enum EClearFlag
{
	Clear_Color		= 0x01,
	Clear_Depth		= 0x02,
	Clear_Stencil	= 0x04,
	Clear_All		= (Clear_Color | Clear_Depth | Clear_Stencil)
};

enum EPrimitiveTopology
{
	PointList,
	LineList,
	LineStrip,
	TriList,
	TriStrip
};

enum ECaps
{
	Caps_VSTex_L16,				// Unsigned short 16-bit texture as a vertex texture
	Caps_VSTexFiltering_Linear	// Bilinear min & mag filtering for vertex textures
};

//!!!fill!
enum EPixelFormat
{
	PixelFmt_Invalid = 0,
	PixelFmt_X8R8G8B8
};

}

#endif
