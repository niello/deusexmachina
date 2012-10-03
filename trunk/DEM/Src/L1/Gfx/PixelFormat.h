#pragma once
#ifndef __DEM_L1_GFX_PIXEL_FORMAT_H__
#define __DEM_L1_GFX_PIXEL_FORMAT_H__

#include <kernel/ntypes.h>
#include <util/nstring.h>

// Pixel formats and related utility functions

namespace Gfx
{

enum EPixelFormat
{
	X8R8G8B8 = 0,
	R8G8B8,
	A8R8G8B8,
	R5G6B5,
	A1R5G5B5,
	A4R4G4B4,
	DXT1,
	DXT3,
	DXT5,
	R16F,                       // 16 bit float, red only
	G16R16F,                    // 32 bit float, 16 bit red, 16 bit green
	A16B16G16R16F,              // 64 bit float, 16 bit rgba each
	R32F,                       // 32 bit float, red only
	G32R32F,                    // 64 bit float, 32 bit red, 32 bit green
	A32B32G32R32F,              // 128 bit float, 32 bit rgba each
	A8,
	X2R10G10B10,
	A2R10G10B10,
	G16R16,
	D24X8,
	D24S8,
	NumPixelFormats,
	InvalidPixelFormat,
};

static EPixelFormat	FromString(const nString& String);
static nString		ToString(EPixelFormat Format);
//???get bpp?

}

#endif