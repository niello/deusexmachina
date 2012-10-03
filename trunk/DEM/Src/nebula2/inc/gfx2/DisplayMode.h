#ifndef N_DISPLAYMODE2_H
#define N_DISPLAYMODE2_H

#include <util/nstring.h>

// Contains display mode parameters.

class CDisplayMode
{
public:

	enum Bpp
	{
		Bpp16,
		Bpp32
	};

private:

	ushort	AASamples; //???to enum?

	//PixelFormat

public:

	ushort	PosX;
	ushort	PosY;
	ushort	Width;
	ushort	Height;
	Bpp		BPP;
	bool	VSync;

	CDisplayMode();
	CDisplayMode(ushort x, ushort y, ushort w, ushort h, bool EnableVSync);

	void	Set(ushort x, ushort y, ushort w, ushort h, bool EnableVSync);

	void	SetAntiAliasSamples(int s) { n_assert(s != 1 && s <= 16); AASamples = s; }
	int		GetAntiAliasSamples() const { return AASamples; }
	float	GetAspectRatio() const { return Width / (float)Height; }
};
//---------------------------------------------------------------------

inline CDisplayMode::CDisplayMode():
	AASamples(0),
	PosX(0),
	PosY(0),
	Width(640),
	Height(480),
	BPP(Bpp32),
	VSync(false)
{
}
//---------------------------------------------------------------------

inline CDisplayMode::CDisplayMode(ushort x, ushort y, ushort w, ushort h, bool EnableVSync):
	PosX(x),
	PosY(y),
	Width(w),
	Height(h),
	BPP(Bpp32),
	VSync(EnableVSync)
{
}
//---------------------------------------------------------------------

inline void CDisplayMode::Set(ushort x, ushort y, ushort w, ushort h, bool EnableVSync)
{
	PosX = x;
	PosY = y;
	Width = w;
	Height = h;
	VSync = EnableVSync;
}
//---------------------------------------------------------------------

#endif
