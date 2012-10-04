#ifndef _VECTOR2_H
#define _VECTOR2_H
//------------------------------------------------------------------------------
/**
    @class _vector2
    @ingroup NebulaMathDataTypes

    Generic vector2 class.

    (C) 2002 RadonLabs GmbH
*/
#include "mathlib/nmath.h"
#include <float.h>

class _vector2
{
public:

	union
	{
		struct { float x, y; };
		float v[2];
	};

	static const _vector2 zero;

	_vector2(): x(0.f), y(0.f) {}
	_vector2(const float _x, const float _y): x(_x), y(_y) {}
	_vector2(const _vector2& vec): x(vec.x), y(vec.y) {}
	_vector2(const float* p): x(p[0]), y(p[1]) {}

	void	set(const float _x, const float _y) { x = _x; y = _y; }
	void	set(const _vector2& v) { x = v.x; y = v.y; }
	void	set(const float* p) { x = p[0]; y = p[1]; }

	float	len() const { return sqrtf(x * x + y * y); }
	float	lensquared() const { return x * x + y * y; }
	void	norm();
	float	dot(const _vector2& v) const { return x * v.x + y * v.y; }
	bool	isequal(const _vector2& v, float tol) const { return fabsf(v.x - x) <= tol && fabsf(v.y - y) <= tol; } //???!!!use fast n_fabs!?
	int		compare(const _vector2& v, float tol) const;
	void	rotate(float angle);
	void	lerp(const _vector2& v0, float lerpVal);
	void	lerp(const _vector2& v0, const _vector2& v1, float lerpVal);
	void	ToLocalAsPoint(const _vector2& Look, const _vector2& Side, const _vector2& Pos);
	void	ToGlobalAsPoint(const _vector2& Look, const _vector2& Side, const _vector2& Pos);
	void	ToLocalAsVector(const _vector2& Look, const _vector2& Side);
	void	ToGlobalAsVector(const _vector2& Look, const _vector2& Side);

	void operator +=(const _vector2& v0) { x += v0.x; y += v0.y; }
	void operator -=(const _vector2& v0) { x -= v0.x; y -= v0.y; }
	void operator *=(const float s) { x *= s; y *= s; }
	void operator /=(float s) { s = 1.f / s; x *= s; y *= s; }
	bool operator ==(const _vector2& rhs) { return x == rhs.x && y == rhs.y; }
	bool operator !=(const _vector2& rhs) { return x != rhs.x || y != rhs.y; }
};
//---------------------------------------------------------------------

inline void _vector2::norm()
{
	float l = len();
	if (l > TINY)
	{
		l = 1.f / l;
		x *= l;
		y *= l;
	}
}
//---------------------------------------------------------------------

inline int _vector2::compare(const _vector2& v, float tol) const
{
	//???!!!use fast n_fabs?!
	if (fabsf(v.x - x) > tol) return (v.x > x) ? +1 : -1;
	else if (fabsf(v.y - y) > tol) return (v.y > y) ? +1 : -1;
	else return 0;
}
//---------------------------------------------------------------------

inline void _vector2::rotate(float angle)
{
	// rotates this one around P(0,0).
	float sa = sinf(angle);
	float ca = cosf(angle);

	// "handmade" multiplication
	_vector2 help(ca * x - sa * y, sa * x + ca * y);

	*this = help;
}
//---------------------------------------------------------------------

inline void _vector2::lerp(const _vector2& v0, float lerpVal)
{
	x = v0.x + ((x - v0.x) * lerpVal);
	y = v0.y + ((y - v0.y) * lerpVal);
}
//---------------------------------------------------------------------

inline void _vector2::lerp(const _vector2& v0, const _vector2& v1, float lerpVal)
{
	x = v0.x + ((v1.x - v0.x) * lerpVal);
	y = v0.y + ((v1.y - v0.y) * lerpVal);
}
//---------------------------------------------------------------------

inline void _vector2::ToLocalAsPoint(const _vector2& Look, const _vector2& Side, const _vector2& Pos)
{
	float TmpX = Look.x * x + Look.y * y - Pos.dot(Look);
	float TmpY = Side.x * x + Side.y * y - Pos.dot(Side);
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

inline void _vector2::ToGlobalAsPoint(const _vector2& Look, const _vector2& Side, const _vector2& Pos)
{
	float TmpX = Look.x * x + Side.x * y + Pos.x;
	float TmpY = Look.y * x + Side.y * y + Pos.y;
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

inline void _vector2::ToLocalAsVector(const _vector2& Look, const _vector2& Side)
{
	float TmpX = Look.x * x + Look.y * y;
	float TmpY = Side.x * x + Side.y * y;
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

inline void _vector2::ToGlobalAsVector(const _vector2& Look, const _vector2& Side)
{
	float TmpX = Look.x * x + Side.x * y;
	float TmpY = Look.y * x + Side.y * y;
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

static inline _vector2 operator +(const _vector2& v0, const _vector2& v1)
{
	return _vector2(v0.x + v1.x, v0.y + v1.y);
}
//---------------------------------------------------------------------

static inline _vector2 operator -(const _vector2& v0, const _vector2& v1)
{
	return _vector2(v0.x - v1.x, v0.y - v1.y);
}
//---------------------------------------------------------------------

static inline _vector2 operator *(const _vector2& v0, const float s)
{
	return _vector2(v0.x * s, v0.y * s);
}
//---------------------------------------------------------------------

static inline _vector2 operator -(const _vector2& v)
{
	return _vector2(-v.x, -v.y);
}
//---------------------------------------------------------------------

#endif

