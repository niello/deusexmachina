#ifndef _VECTOR3_H
#define _VECTOR3_H

#include <Math/Math.h>
#include <float.h>

// Generic vector3 class. SSE-unfriendly, 12-byte size.
// (C) 2002 RadonLabs GmbH

class vector4;

class vector3
{
public:

	static const vector3 Zero;
	static const vector3 One;
	static const vector3 Up;
	static const vector3 AxisX;
	static const vector3 AxisY;
	static const vector3 AxisZ;
	static const vector3 BaseDir;

	union
	{
		struct { float x, y, z; };
		float v[3];
	};

	vector3(): x(0.f), y(0.f), z(0.f) {}
	vector3(const float _x, const float _y, const float _z): x(_x), y(_y), z(_z) {}
	vector3(const vector3& vec): x(vec.x), y(vec.y), z(vec.z) {}
	vector3(const vector4& vec);
	vector3(const float* vec): x(vec[0]), y(vec[1]), z(vec[2]) {}

	static float	Distance(const vector3& v0, const vector3& v1) { return n_sqrt(SqDistance(v0, v1)); }
	static float	Distance2D(const vector3& v0, const vector3& v1) { return n_sqrt(SqDistance2D(v0, v1)); }
	static float	SqDistance(const vector3& v0, const vector3& v1);
	static float	SqDistance2D(const vector3& v0, const vector3& v1);
	static float	Angle(const vector3& v0, const vector3& v1);
	static float	Angle2D(const vector3& v0, const vector3& v1);
	static float	AngleNorm(const vector3& v0n, const vector3& v1n);
	static float	Angle2DNorm(const vector3& v0n, const vector3& v1n);

	void			set(const float _x, const float _y, const float _z) { x = _x; y = _y; z = _z; }
	void			set(const vector3& vec) { x = vec.x; y = vec.y; z = vec.z; }
	void			set(const float* vec) { x = vec[0]; y = vec[1]; z = vec[2]; }
	float			Length() const { return n_sqrt(x * x + y * y + z * z); }
	float			Length2D() const { return n_sqrt(x * x + z * z); }
	float			SqLength() const { return x * x + y * y + z * z; }
	float			SqLength2D() const { return x * x + z * z; }
	void			norm();

	bool			isequal(const vector3& v, float tol) const { return n_fabs(v.x - x) <= tol && n_fabs(v.y - y) <= tol && n_fabs(v.z - z) <= tol; }
	int				compare(const vector3& v, float tol) const;
	void			rotate(const vector3& axis, float angle);
	void			lerp(const vector3& v0, float lerpVal);
	void			lerp(const vector3& v0, const vector3& v1, float lerpVal);
	vector3			findortho() const;
	void			saturate() { x = Saturate(x); y = Saturate(y); z = Saturate(z); }
	float			Dot(const vector3& v0) const { return x * v0.x + y * v0.y + z * v0.z; }
	float			Dot2D(const vector3& v0) const { return x * v0.x + z * v0.z; }

	void operator +=(const vector3& v0) { x += v0.x; y += v0.y; z += v0.z; }
	void operator -=(const vector3& v0) { x -= v0.x; y -= v0.y; z -= v0.z; }
	void operator *=(const vector3& v0) { x *= v0.x; y *= v0.y; z *= v0.z; }
	void operator /=(const vector3& v0) { x /= v0.x; y /= v0.y; z /= v0.z; }
	void operator *=(float s) { x *= s; y *= s; z *= s; }
	void operator /=(float s) { s = 1.f / s; x *= s; y *= s; z *= s; }
	bool operator >(const vector3& rhs) const { return x > rhs.x || y > rhs.y || z > rhs.z; }
	bool operator <(const vector3& rhs) const { return x < rhs.x || y < rhs.y || z < rhs.z; }
	bool operator ==(const vector3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
	bool operator !=(const vector3& rhs) const { return x != rhs.x || y != rhs.y || z != rhs.z; }
	bool operator ==(const float* v) const { return x == v[0] && y == v[1] && z == v[2]; }
	bool operator !=(const float* v) const { return x != v[0] || y != v[1] || z != v[2]; }
};

static inline vector3 operator -(const vector3& v)
{
	return vector3(-v.x, -v.y, -v.z);
}
//---------------------------------------------------------------------

static inline vector3 operator +(const vector3& v0, const vector3& v1)
{
	return vector3(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
}
//---------------------------------------------------------------------

static inline vector3 operator -(const vector3& v0, const vector3& v1)
{
	return vector3(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
}
//---------------------------------------------------------------------

static inline vector3 operator *(const vector3& v0, const float s)
{
	return vector3(v0.x * s, v0.y * s, v0.z * s);
}
//---------------------------------------------------------------------

static inline vector3 operator /(const vector3& v0, float s)
{
	s = 1.0f / s;
	return vector3(v0.x * s, v0.y * s, v0.z * s);
}
//---------------------------------------------------------------------

//???force to method? not operator?
// Dot product.
static inline float operator %(const vector3& v0, const vector3& v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}
//---------------------------------------------------------------------

//???force to method? not operator?
// Cross product.
static inline vector3 operator *(const vector3& v0, const vector3& v1)
{
	return vector3(v0.y * v1.z - v0.z * v1.y,
					v0.z * v1.x - v0.x * v1.z,
					v0.x * v1.y - v0.y * v1.x);
}
//---------------------------------------------------------------------

inline void vector3::norm()
{
	float l = Length();
	if (l > TINY)
	{
		l = 1.f / l;
		x *= l;
		y *= l;
		z *= l;
	}
}
//---------------------------------------------------------------------

inline int vector3::compare(const vector3& v, float tol) const
{
	if (n_fabs(v.x - x) > tol) return (v.x > x) ? +1 : -1;
	else if (n_fabs(v.y - y) > tol) return (v.y > y) ? +1 : -1;
	else if (n_fabs(v.z - z) > tol) return (v.z > z) ? +1 : -1;
	else return 0;
}
//---------------------------------------------------------------------

inline void vector3::rotate(const vector3& axis, float angle)
{
	// rotates this one around given vector. We do
	// rotation with matrices, but these aren't defined yet!
	float rotM[9];
	float sa, ca;
	n_sincos(angle, sa, ca);

	// build a rotation matrix
	rotM[0] = ca + (1 - ca) * axis.x * axis.x;
	rotM[1] = (1 - ca) * axis.x * axis.y - sa * axis.z;
	rotM[2] = (1 - ca) * axis.z * axis.x + sa * axis.y;
	rotM[3] = (1 - ca) * axis.x * axis.y + sa * axis.z;
	rotM[4] = ca + (1 - ca) * axis.y * axis.y;
	rotM[5] = (1 - ca) * axis.y * axis.z - sa * axis.x;
	rotM[6] = (1 - ca) * axis.z * axis.x - sa * axis.y;
	rotM[7] = (1 - ca) * axis.y * axis.z + sa * axis.x;
	rotM[8] = ca + (1 - ca) * axis.z * axis.z;

	// "handmade" multiplication
	vector3 help(	rotM[0] * x + rotM[1] * y + rotM[2] * z,
					rotM[3] * x + rotM[4] * y + rotM[5] * z,
					rotM[6] * x + rotM[7] * y + rotM[8] * z);
	*this = help;
}
//---------------------------------------------------------------------

inline void vector3::lerp(const vector3& v0, float lerpVal)
{
	x = v0.x + ((x - v0.x) * lerpVal);
	y = v0.y + ((y - v0.y) * lerpVal);
	z = v0.z + ((z - v0.z) * lerpVal);
}
//---------------------------------------------------------------------

inline void vector3::lerp(const vector3& v0, const vector3& v1, float lerpVal)
{
	x = v0.x + ((v1.x - v0.x) * lerpVal);
	y = v0.y + ((v1.y - v0.y) * lerpVal);
	z = v0.z + ((v1.z - v0.z) * lerpVal);
}
//---------------------------------------------------------------------

// Find a vector that is orthogonal to self. Self should not be (0,0,0).
// Return value is not normalized.
inline vector3 vector3::findortho() const
{
	if (x) return vector3((-y - z) / x, 1.f, 1.f);
	else if (y) return vector3(1.f, (-x - z) / y, 1.f);
	else if (z) return vector3(1.f, 1.f, (-x - y) / z);
	else return vector3(0.f, 0.f, 0.f);
}
//---------------------------------------------------------------------

inline float vector3::SqDistance(const vector3& v0, const vector3& v1)
{
	vector3 v(v1 - v0);
	return v.x * v.x + v.y * v.y + v.z * v.z;
}
//---------------------------------------------------------------------

inline float vector3::SqDistance2D(const vector3& v0, const vector3& v1)
{
	float vx = v1.x - v0.x;
	float vz = v1.z - v0.z;
	return vx * vx + vz * vz;
}
//---------------------------------------------------------------------

inline float vector3::Angle(const vector3& v0, const vector3& v1)
{
	vector3 v0n = v0;
	vector3 v1n = v1;
	v0n.norm();
	v1n.norm();
	return AngleNorm(v0n, v1n);
}
//---------------------------------------------------------------------

inline float vector3::AngleNorm(const vector3& v0n, const vector3& v1n)
{
	return n_acos(v0n % v1n);
}
//---------------------------------------------------------------------

inline float vector3::Angle2D(const vector3& v0, const vector3& v1)
{
	vector3 v0n = v0;
	vector3 v1n = v1;
	v0n.norm();
	v1n.norm();
	return Angle2DNorm(v0n, v1n);
}
//---------------------------------------------------------------------

inline float vector3::Angle2DNorm(const vector3& v0n, const vector3& v1n)
{
	// Since angle is calculated in XZ plane, we simplify our calculations.
	// Originally SinA = CrossY = (v0n x v1n) * XZPlaneNormal
	// and CosA = Dot = v0n * v1n
	// XZPlaneNormal = Up = (0, 1, 0), so we need only Y component of Cross.
	float CrossY = v0n.z * v1n.x - v0n.x * v1n.z;
	float Dot = v0n.x * v1n.x + v0n.z * v1n.z;
	return atan2f(CrossY, Dot);
}
//---------------------------------------------------------------------

template<> static inline void lerp<vector3>(vector3 & result, const vector3 & val0, const vector3 & val1, float lerpVal)
{
	result.lerp(val0, val1, lerpVal);
}
//---------------------------------------------------------------------

#endif
