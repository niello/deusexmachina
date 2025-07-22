#pragma once
#include <Render/RenderFwd.h>

// Display mode of some video adapter output. Use in CDisplayDriver to
// change application display mode for curtain monitor.

namespace Render
{

class CDisplayMode
{
public:

	CRational		RefreshRate;
	EPixelFormat	PixelFormat;
	U16				Width;
	U16				Height;
	//EScanLineOrder;	// Use "unspecified" where can't obtain
	//EDisplayScaling;	// Use "unspecified" where can't obtain
	bool			Stereo = false;

	CDisplayMode(U16 w = 1024, U16 h = 768, EPixelFormat Format = PixelFmt_Invalid);

	float	GetAspectRatio() const { return Width / static_cast<float>(Height); }
	void	GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const;
	void	GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const;

	bool	operator ==(const CDisplayMode& Other) const;
	bool	operator !=(const CDisplayMode& Other) const { return !(*this == Other); }
	bool	operator <(const CDisplayMode& Other) const;
};

inline CDisplayMode::CDisplayMode(U16 w, U16 h, EPixelFormat Format):
	Width(w),
	Height(h),
	PixelFormat(Format),
	RefreshRate{0, 1}
{
}
//---------------------------------------------------------------------

inline void CDisplayMode::GetAbsoluteXY(float XRel, float YRel, int& XAbs, int& YAbs) const
{
	XAbs = static_cast<int>(XRel * Width);
	YAbs = static_cast<int>(YRel * Height);
}
//---------------------------------------------------------------------

inline void CDisplayMode::GetRelativeXY(int XAbs, int YAbs, float& XRel, float& YRel) const
{
	XRel = XAbs / static_cast<float>(Width);
	YRel = YAbs / static_cast<float>(Height);
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

inline bool CDisplayMode::operator <(const CDisplayMode& Other) const
{
	if (Width != Other.Width) return Width < Other.Width;
	if (Height != Other.Height) return Height < Other.Height;

	const I32 RR = RefreshRate.GetIntRounded();
	const I32 OtherRR = Other.RefreshRate.GetIntRounded();
	if (RR != OtherRR) return RR < OtherRR;

	return PixelFormat < Other.PixelFormat;
}
//---------------------------------------------------------------------

}
