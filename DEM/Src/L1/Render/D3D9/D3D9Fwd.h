#pragma once
#ifndef __DEM_L1_RENDER_D3D9_FWD_H__
#define __DEM_L1_RENDER_D3D9_FWD_H__

#include <Data/StringID.h>
#include <Data/FixedArray.h>

// Direct3D9 render implementation forward declaration

namespace Render
{

// Don't change values
enum ESM30RegisterSet
{
	Reg_Bool			= 0,
	Reg_Int4			= 1,
	Reg_Float4			= 2,

	Reg_Invalid
};

// Don't change values
enum ESM30SamplerType
{
	SM30Sampler_1D		= 0,
	SM30Sampler_2D,
	SM30Sampler_3D,
	SM30Sampler_CUBE
};

}

#endif
