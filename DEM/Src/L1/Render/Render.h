#pragma once
#ifndef __DEM_L1_RENDER_H__
#define __DEM_L1_RENDER_H__

#include <Render/D3D9Fwd.h>
#include <kernel/ntypes.h>

// Render system definitions and forward declarations

namespace Render
{

enum EMSAAQuality
{
	MSAA_None	= 0,
	MSAA_2x		= 2,
	MSAA_4x		= 4,
	MSAA_8x		= 8
};

struct CMonitorInfo
{
	ushort	Left;
	ushort	Top;
	ushort	Width;
	ushort	Height;
	bool	IsPrimary;
};

}

#endif
