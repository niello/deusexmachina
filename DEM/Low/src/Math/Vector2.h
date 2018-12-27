#ifndef _VECTOR2_H
#define _VECTOR2_H
//------------------------------------------------------------------------------
/**
    @class vector2
    @ingroup NebulaMathDataTypes

    Generic vector2 class.

    (C) 2002 RadonLabs GmbH
*/
#include "Math/Math.h"
#include <float.h>

class vector2
{
public:

	union
	{
		struct { float x, y; };
		float v[2];
	};

	static const vector2 zero;

	vector2(): x(0.f), y(0.f) {}
	vector2(const float _x, const float _y): x(_x), y(_y) {}
	vector2(const vector2& vec): x(vec.x), y(vec.y) {}
	vector2(const float* p): x(p[0]), y(p[1]) {}

	void	set(const float _x, const float _y) { x = _x; y = _y; }
	void	set(const vector2& v) { x = v.x; y = v.y; }
	void	set(const float* p) { x = p[0]; y = p[1]; }

	float	Length() const { return n_sqrt(x * x + y * y); }
	float	SqLength() const { return x * x + y * y; }
	void	norm();
	float	dot(const vector2& v) const { return x * v.x + y * v.y; }
	bool	isequal(const vector2& v, float tol) const { return n_fabs(v.x - x) <= tol && n_fabs(v.y - y) <= tol; } //???!!!use fast n_fabs!?
	int		compare(const vector2& v, float tol) const;
	void	rotate(float angle);
	void	lerp(const vector2& v0, float lerpVal);
	void	lerp(const vector2& v0, const vector2& v1, float lerpVal);
	void	ToLocalAsPoint(const vector2& Look, const vector2& Side, const vector2& Pos);
	void	ToGlobalAsPoint(const vector2& Look, const vector2& Side, const vector2& Pos);
	void	ToLocalAsVector(const vector2& Look, const vector2& Side);
	void	ToGlobalAsVector(const vector2& Look, const vector2& Side);

	void operator +=(const vector2& v0) { x += v0.x; y += v0.y; }
	void operator -=(const vector2& v0) { x -= v0.x; y -= v0.y; }
	void operator *=(const float s) { x *= s; y *= s; }
	void operator /=(float s) { s = 1.f / s; x *= s; y *= s; }
	bool operator ==(const vector2& rhs) { return x == rhs.x && y == rhs.y; }
	bool operator !=(const vector2& rhs) { return x != rhs.x || y != rhs.y; }
};
//---------------------------------------------------------------------

inline void vector2::norm()
{
	float l = Length();
	if (l > TINY)
	{
		l = 1.f / l;
		x *= l;
		y *= l;
	}
}
//---------------------------------------------------------------------

inline int vector2::compare(const vector2& v, float tol) const
{
	//???!!!use fast n_fabs?!
	if (n_fabs(v.x - x) > tol) return (v.x > x) ? +1 : -1;
	else if (n_fabs(v.y - y) > tol) return (v.y > y) ? +1 : -1;
	else return 0;
}
//---------------------------------------------------------------------

inline void vector2::rotate(float angle)
{
	// rotates this one around P(0,0).
	float sa = sinf(angle);
	float ca = cosf(angle);

	// "handmade" multiplication
	vector2 help(ca * x - sa * y, sa * x + ca * y);

	*this = help;
}
//---------------------------------------------------------------------

inline void vector2::lerp(const vector2& v0, float lerpVal)
{
	x = v0.x + ((x - v0.x) * lerpVal);
	y = v0.y + ((y - v0.y) * lerpVal);
}
//---------------------------------------------------------------------

inline void vector2::lerp(const vector2& v0, const vector2& v1, float lerpVal)
{
	x = v0.x + ((v1.x - v0.x) * lerpVal);
	y = v0.y + ((v1.y - v0.y) * lerpVal);
}
//---------------------------------------------------------------------

inline void vector2::ToLocalAsPoint(const vector2& Look, const vector2& Side, const vector2& Pos)
{
	float TmpX = Look.x * x + Look.y * y - Pos.dot(Look);
	float TmpY = Side.x * x + Side.y * y - Pos.dot(Side);
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

inline void vector2::ToGlobalAsPoint(const vector2& Look, const vector2& Side, const vector2& Pos)
{
	float TmpX = Look.x * x + Side.x * y + Pos.x;
	float TmpY = Look.y * x + Side.y * y + Pos.y;
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

inline void vector2::ToLocalAsVector(const vector2& Look, const vector2& Side)
{
	float TmpX = Look.x * x + Look.y * y;
	float TmpY = Side.x * x + Side.y * y;
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

inline void vector2::ToGlobalAsVector(const vector2& Look, const vector2& Side)
{
	float TmpX = Look.x * x + Side.x * y;
	float TmpY = Look.y * x + Side.y * y;
	x = TmpX;
	y = TmpY;
}
//---------------------------------------------------------------------

static inline vector2 operator +(const vector2& v0, const vector2& v1)
{
	return vector2(v0.x + v1.x, v0.y + v1.y);
}
//---------------------------------------------------------------------

static inline vector2 operator -(const vector2& v0, const vector2& v1)
{
	return vector2(v0.x - v1.x, v0.y - v1.y);
}
//---------------------------------------------------------------------

static inline vector2 operator *(const vector2& v0, const float s)
{
	return vector2(v0.x * s, v0.y * s);
}
//---------------------------------------------------------------------

static inline vector2 operator -(const vector2& v)
{
	return vector2(-v.x, -v.y);
}
//---------------------------------------------------------------------

template<> static inline void lerp<vector2>(vector2 & result, const vector2 & val0, const vector2 & val1, float lerpVal)
{
	result.lerp(val0, val1, lerpVal);
}
//---------------------------------------------------------------------

#endif

