#pragma once
#ifndef __DEM_L1_RENDER_DISPLAY_MODE_H__
#define __DEM_L1_RENDER_DISPLAY_MODE_H__

#include <Render/RenderFwd.h>
#include <Data/String.h>

// Display mode of some video adapter output. Use in CDisplayDriver to
// change application display mode for curtain monitor.

namespace Render
{

class CDisplayMode
{
public:

	CRational		RefreshRate;
	EPixelFormat	PixelFormat;
	U16			Width;
	U16			Height;
	//EScanLineOrder;	// Use "unspecified" where can't obtain
	//EDisplayScaling;	// Use "unspecified" where can't obtain
	bool			Stereo;

	CDisplayMode(U16 w = 1024, U16 h = 768, EPixelFormat Format = PixelFmt_Invalid);

	float	GetAspectRatio() const { return Width / (float)Height; }
	void	GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const;
	void	GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	bool	operator ==(const CDisplayMode& Other) const;
	bool	operator !=(const CDisplayMode& Other) const;
};

inline CDisplayMode::CDisplayMode(U16 w, U16 h, EPixelFormat Format):
	Width(w),
	Height(h),
	PixelFormat(Format),
	Stereo(false)
{
	RefreshRate.Numerator = 0;
	RefreshRate.Denominator = 1;
}
//---------------------------------------------------------------------

inline void CDisplayMode::GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const
{
	XAbs = (int)(XRel * Width);
	YAbs = (int)(YRel * Height);
}
//---------------------------------------------------------------------

inline void CDisplayMode::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	XRel = XAbs / float(Width);
	YRel = YAbs / float(Height);
}
//---------------------------------------------------------------------

inline bool CDisplayMode::operator ==(const CDisplayMode& Other) const
{
	return Width == Other.Width &&
		Height == Other.Height &&
		PixelFormat == Other.PixelFormat &&
		RefreshRate == Other.RefreshRate;
}
//---------------------------------------------------------------------

inline bool CDisplayMode::operator !=(const CDisplayMode& Other) const
{
	return Width != Other.Width ||
		Height != Other.Height ||
		PixelFormat != Other.PixelFormat ||
		RefreshRate != Other.RefreshRate;
}
//---------------------------------------------------------------------

}

#endif
