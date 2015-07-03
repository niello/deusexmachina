#pragma once
#ifndef __DEM_L1_REGIONS_H__
#define __DEM_L1_REGIONS_H__

// Line 1D, rect 2D and cuboid 3D axis-aligned region descriptions.
// Implicit constructors allow to interchange these objects.

namespace Data
{
struct CRect;
struct CBox;

struct CSegment
{
	int				X;
	unsigned int	W;

	CSegment(): X(0), W(0) {}
	CSegment(int x, unsigned int w): X(x), W(w) {}
	CSegment(const CRect& Rect);
	CSegment(const CBox& Box);

	int	Left() const { return X; }
	int	Right() const { return X + W; }
};

struct CRect
{
	int				X, Y;
	unsigned int	W, H;

	CRect(): X(0), Y(0), W(0), H(0) {}
	CRect(int x, int y, unsigned int w, unsigned int h): X(x), Y(y), W(w), H(h) {}
	CRect(const CSegment& Seg): X(Seg.X), Y(0), W(Seg.W), H(0) {}
	CRect(const CBox& Box);

	int	Left() const { return X; }
	int	Top() const { return Y; }
	int	Right() const { return X + W; }
	int	Bottom() const { return Y + H; }
};

struct CBox
{
	int				X, Y, Z;
	unsigned int	W, H, D;

	CBox(): X(0), Y(0), Z(0), W(0), H(0), D(0) {}
	CBox(int x, int y, int z, unsigned int w, unsigned int h, unsigned int d): X(x), Y(y), Z(z), W(w), H(h), D(d) {}
	CBox(const CSegment& Seg): X(Seg.X), Y(0), Z(0), W(Seg.W), H(0), D(0) {}
	CBox(const CRect& Rect): X(Rect.X), Y(Rect.Y), Z(0), W(Rect.W), H(Rect.H), D(0) {}

	int	Left() const { return X; }
	int	Top() const { return Y; }
	int	Front() const { return Z; }
	int	Right() const { return X + W; }
	int	Bottom() const { return Y + H; }
	int	Back() const { return Z + D; }
};

inline CSegment::CSegment(const CRect& Rect): X(Rect.X), W(Rect.W) {}
inline CSegment::CSegment(const CBox& Box): X(Box.X), W(Box.W) {}
inline CRect::CRect(const CBox& Box): X(Box.X), Y(Box.Y), W(Box.W), H(Box.H) {}

}

#endif
