#pragma once

// Line 1D, rect 2D and cuboid 3D axis-aligned region descriptions.
// Implicit constructors allow to convert between these objects.

//!!!make templated!

namespace Data
{
struct CRect;
struct CBox;

//template<class TPos, class TSize = TPos>
struct CSegment
{
	IPTR X;
	UPTR W;

	CSegment(): X(0), W(0) {}
	CSegment(IPTR x, UPTR w): X(x), W(w) {}
	CSegment(const CRect& Rect);
	CSegment(const CBox& Box);

	IPTR Left() const { return X; }
	IPTR Right() const { return X + W; }
};

struct CRect
{
	IPTR X, Y;
	UPTR W, H;

	CRect(): X(0), Y(0), W(0), H(0) {}
	CRect(IPTR x, IPTR y, UPTR w, UPTR h): X(x), Y(y), W(w), H(h) {}
	CRect(const CSegment& Seg): X(Seg.X), Y(0), W(Seg.W), H(0) {}
	CRect(const CBox& Box);

	IPTR Left() const { return X; }
	IPTR Top() const { return Y; }
	IPTR Right() const { return X + W; }
	IPTR Bottom() const { return Y + H; }

	bool Contains(IPTR x, IPTR y) const { return x >= X && y >= y && x <= Right() && y <= Bottom(); }
};

struct CRectF //???use templates?
{
	float X, Y, W, H;

	CRectF(): X(0), Y(0), W(0), H(0) {}
	CRectF(float x, float y, float w, float h): X(x), Y(y), W(w), H(h) {}

	float Left() const { return X; }
	float Top() const { return Y; }
	float Right() const { return X + W; }
	float Bottom() const { return Y + H; }
};

struct CBox
{
	IPTR X, Y, Z;
	UPTR W, H, D;

	CBox(): X(0), Y(0), Z(0), W(0), H(0), D(0) {}
	CBox(IPTR x, IPTR y, IPTR z, UPTR w, UPTR h, UPTR d): X(x), Y(y), Z(z), W(w), H(h), D(d) {}
	CBox(const CSegment& Seg): X(Seg.X), Y(0), Z(0), W(Seg.W), H(0), D(0) {}
	CBox(const CRect& Rect): X(Rect.X), Y(Rect.Y), Z(0), W(Rect.W), H(Rect.H), D(0) {}

	IPTR Left() const { return X; }
	IPTR Top() const { return Y; }
	IPTR Front() const { return Z; }
	IPTR Right() const { return X + W; }
	IPTR Bottom() const { return Y + H; }
	IPTR Back() const { return Z + D; }
};

inline CSegment::CSegment(const CRect& Rect): X(Rect.X), W(Rect.W) {}
inline CSegment::CSegment(const CBox& Box): X(Box.X), W(Box.W) {}
inline CRect::CRect(const CBox& Box): X(Box.X), Y(Box.Y), W(Box.W), H(Box.H) {}

}
