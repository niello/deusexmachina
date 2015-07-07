#pragma once
#ifndef __DEM_L1_RENDER_IMAGE_UTILS_H__
#define __DEM_L1_RENDER_IMAGE_UTILS_H__

#include <StdDEM.h>

// API-independent image processing and attribute calculation utility functions

namespace Data
{
	struct CBox;
}

namespace Render
{
struct CMappedTexture;

enum ECopyImageFlags
{
	CopyImage_AdjustDest		= 0x01,	// States that dest pointer is a pointer to image start and must be adjusted to offset
	CopyImage_BlockCompressed	= 0x02,	// Data is in DXT or BC format and therefore is packed into 4x4 pixel blocks
	CopyImage_3DImage			= 0x03	// Source and destination are 3D images (volumes)
};

struct CCopyImageParams
{
	DWORD BitsPerPixel;		// For both source and destination formats
	DWORD Offset[3];		// Destination offset X, Y, Z in pixels
	DWORD CopySize[3];		// Size to copy X, Y, Z in pixels
	DWORD TotalSize[2];		// Destination dimensions X, Y (to detect whole slice / whole row copy)
};

// Source and destination formats must match, no conversion occurs //!!!can use custom memcpy substitutes as arg to allow conversion algorithms!
void __fastcall CopyImage(const CMappedTexture& Src, const CMappedTexture& Dest, DWORD Flags, const CCopyImageParams& Params);

// pInRegion may be NULL, which means that region covers the whole image
// Dimensions is a number of image dimensions from 1 to 3
// If region requested is degenerate, function returns false not finishing calculations
bool __fastcall CalcValidImageRegion(const Data::CBox* pInRegion, DWORD Dimensions,
									 DWORD ImageWidth, DWORD ImageHeight, DWORD ImageDepth,
									 DWORD& OutOffsetX, DWORD& OutOffsetY, DWORD& OutOffsetZ,
									 DWORD& OutSizeX, DWORD& OutSizeY, DWORD& OutSizeZ);
}

#endif
