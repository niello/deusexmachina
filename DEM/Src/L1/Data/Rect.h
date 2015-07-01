#pragma once
#ifndef __DEM_L1_RECT_H__
#define __DEM_L1_RECT_H__

// Rectangular 2D region description

namespace Data
{

struct CRect
{
	int				X, Y;
	unsigned int	W, H;

	CRect(): X(0), Y(0), W(0), H(0) {}
	CRect(int x, int y, unsigned int w, unsigned int h): X(x), Y(y), W(w), H(h) {}

	int	Left() const { return X; }
	int	Top() const { return Y; }
	int	Right() const { return X + W; }
	int	Bottom() const { return Y + H; }
};

}

#endif
