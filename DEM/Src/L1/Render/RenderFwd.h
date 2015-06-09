#pragma once
#ifndef __DEM_L1_RENDER_H__
#define __DEM_L1_RENDER_H__

#include <StdDEM.h>

// Render system definitions and forward declarations

#define DEM_RENDER_DEBUG (0)
#define DEM_RENDER_USENVPERFHUD (0)

namespace Render
{
typedef DWORD HShaderParam; // Opaque to user, so its internal meaning can be different for different APIs
struct CShaderConstantDesc;

const DWORD Adapter_AutoSelect = (DWORD)-2;
const DWORD Adapter_None = (DWORD)-1;
const DWORD Adapter_Primary = 0;
const DWORD Adapter_Secondary = 1;
const DWORD Output_None = (DWORD)-1;

enum EGPUDriverType
{
	// Prefers hardware driver when possible and falls back to reference device.
	// Use it only as a creation parameter and never as an actual driver type.
	GPU_AutoSelect = 0,

	GPU_Hardware,	// Real hardware device
	GPU_Reference,	// Software emulation (for debug purposes)
	GPU_Software,	// Pluggable software driver
	GPU_Null		// No rendering (for non-rendering API calls verification)
};

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
//???reverse component order? see DXGI formats
enum EPixelFormat
{
	PixelFmt_Invalid = 0,
	PixelFmt_X8R8G8B8,
	PixelFmt_A8R8G8B8,
	PixelFmt_R5G6B5
};

enum EMSAAQuality
{
	MSAA_None	= 0,
	MSAA_2x		= 2,
	MSAA_4x		= 4,
	MSAA_8x		= 8
};

// Error codes
#define ERR_CREATION_ERROR ((DWORD)-1);
#define ERR_DRIVER_TYPE_NOT_SUPPORTED ((DWORD)-2);

}

#endif
