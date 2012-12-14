#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_MODE_H__
#define __DEM_L1_RENDER_DISPLAY_MODE_H__

#include <Render/Render.h>
#include <util/nstring.h>

// Contains display mode parameters.

class CDisplayMode
{
public:

	//!!!OLD!
	enum EBitDepth
	{
		BPP_16 = 16,
		BPP_32 = 32
	};

	EBitDepth		BPP;	//???pixel format instead?
	//!!!OLD!

	ushort			PosX;
	ushort			PosY;
	ushort			Width;
	ushort			Height;
	EPixelFormat	PixelFormat;

	CDisplayMode();
	CDisplayMode(ushort x, ushort y, ushort w, ushort h);
	CDisplayMode(ushort w, ushort h, EPixelFormat Format);

	float	GetAspectRatio() const { return Width / (float)Height; }

	bool	operator ==(const CDisplayMode& Other) const;
};
//---------------------------------------------------------------------

inline CDisplayMode::CDisplayMode():
	PosX(0),
	PosY(0),
	Width(1024),
	Height(768),
	BPP(BPP_32),
	PixelFormat(PixelFormat_Invalid) //X8R8G8B8)
{
}
//---------------------------------------------------------------------

inline CDisplayMode::CDisplayMode(ushort x, ushort y, ushort w, ushort h):
	PosX(x),
	PosY(y),
	Width(w),
	Height(h),
	BPP(BPP_32),
	PixelFormat(PixelFormat_Invalid)
{
}
//---------------------------------------------------------------------

inline CDisplayMode::CDisplayMode(ushort w, ushort h, EPixelFormat Format):
	PosX(0),
	PosY(0),
	Width(w),
	Height(h),
	BPP(BPP_32),
	PixelFormat(Format)
{
}
//---------------------------------------------------------------------

inline bool CDisplayMode::operator ==(const CDisplayMode& Other) const
{
	return PosX == Other.PosX &&
		PosY == Other.PosY &&
		Width == Other.Width &&
		Height == Other.Height &&
		PixelFormat == Other.PixelFormat;
}
//---------------------------------------------------------------------

#endif
