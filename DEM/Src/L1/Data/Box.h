#pragma once
#ifndef __DEM_L1_BOX_H__
#define __DEM_L1_BOX_H__

// Cuboid 3D region description

namespace Data
{

struct CBox
{
	int				X, Y, Z;
	unsigned int	W, H, D;

	CBox(): X(0), Y(0), Z(0), W(0), H(0), D(0) {}
	CBox(int x, int y, int z, unsigned int w, unsigned int h, unsigned int d): X(x), Y(y), Z(z), W(w), H(h), D(d) {}

	int	Left() const { return X; }
	int	Top() const { return Y; }
	int	Near() const { return Z; }
	int	Right() const { return X + W; }
	int	Bottom() const { return Y + H; }
	int	Far() const { return Z + D; }
};

}

#endif
