#ifndef _VECTOR4_H
#define _VECTOR4_H
//------------------------------------------------------------------------------
/**
    @class vector4
    @ingroup NebulaMathDataTypes

    A generic vector4 class.

    (C) 2002 RadonLabs GmbH
*/
#include "mathlib/nmath.h"
#include <float.h>
#include "mathlib/vector3.h"

//------------------------------------------------------------------------------
class vector4
{
public:

	static const vector4 Zero;
	static const vector4 Red;
	static const vector4 Green;
	static const vector4 Blue;
	static const vector4 White;

public:
    enum component
    {
        X = (1<<0),
        Y = (1<<1),
        Z = (1<<2),
        W = (1<<3),
    };

    /// constructor 1
    vector4();
    /// constructor 2
    vector4(const float _x, const float _y, const float _z, const float _w);
    /// constructor 3
    vector4(const vector4& vec);
    /// constructor from vector3 (w will be set to 1.0)
    vector4(const vector3& vec3);
    /// set elements 1
    void set(const float _x, const float _y, const float _z, const float _w);
    /// set elements 2
    void set(const vector4& v);
    /// set to vector3 (w will be set to 1.0)
    void set(const vector3& v);
    /// return length
    float len() const;
    /// normalize
    void norm();
    /// inplace add
    void operator +=(const vector4& v);
    /// inplace sub
    void operator -=(const vector4& v);
    /// inplace scalar mul
    void operator *=(const float s);
    /// inplace scalar div
    void operator /=(float s);
    /// true if all elements are equal
    bool operator ==(const vector4& v0);
    /// true if any of the elements is not equal
    bool operator !=(const vector4& v0);
    /// vector3 assignment operator (w set to 1.0f)
    vector4& operator=(const vector3& v);
    /// fuzzy compare
    bool isequal(const vector4& v, float tol) const;
    /// fuzzy compare, return -1, 0, +1
    int compare(const vector4& v, float tol) const;
    /// set own components to minimum
    void minimum(const vector4& v);
    /// set own components to maximum
    void maximum(const vector4& v);
    /// set component float value by mask
    void setcomp(float val, int mask);
    /// get component float value by mask
    float getcomp(int mask);
    /// get write mask for smallest component
    int mincompmask() const;
    /// inplace linear interpolation
    void lerp(const vector4& v0, float lerpVal);
    /// linear interpolation between v0 and v1
    void lerp(const vector4& v0, const vector4& v1, float lerpVal);
    /// saturate components between 0 and 1
    void saturate();
    /// dot product
    float dot(const vector4& v0) const;

	union
	{
		struct { float x, y, z, w; };
		float v[4];
	};
};

//------------------------------------------------------------------------------
/**
*/
inline
vector4::vector4() :
    x(0.0f),
    y(0.0f),
    z(0.0f),
    w(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
vector4::vector4(const float _x, const float _y, const float _z, const float _w) :
    x(_x),
    y(_y),
    z(_z),
    w(_w)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
vector4::vector4(const vector4& v) :
    x(v.x),
    y(v.y),
    z(v.z),
    w(v.w)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
vector4::vector4(const vector3& v) :
    x(v.x),
    y(v.y),
    z(v.z),
    w(1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::set(const float _x, const float _y, const float _z, const float _w)
{
    x = _x;
    y = _y;
    z = _z;
    w = _w;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::set(const vector4& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::set(const vector3& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
    w = 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
vector4::len() const
{
    return (float) sqrt(x * x + y * y + z * z + w * w);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::norm()
{
    float l = len();
    if (l > TINY)
    {
        float oneDivL = 1.0f / l;
        x *= oneDivL;
        y *= oneDivL;
        z *= oneDivL;
        w *= oneDivL;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::operator +=(const vector4& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    w += v.w;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::operator -=(const vector4& v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    w -= v.w;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::operator *=(const float s)
{
    x *= s;
    y *= s;
    z *= s;
    w *= s;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::operator /=(float s)
{
	s = 1.f / s;
    x *= s;
    y *= s;
    z *= s;
    w *= s;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
vector4::operator ==(const vector4& rhs)
{
    if ((this->x == rhs.x) && (this->y == rhs.y) && (this->z == rhs.z) && (this->w == rhs.w))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
vector4::operator !=(const vector4& rhs)
{
    if ((this->x != rhs.x) || (this->y != rhs.y) || (this->z != rhs.z) || (this->w != rhs.w))
    {
        return true;
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
vector4&
vector4::operator=(const vector3& v)
{
    this->set(v);
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
vector4::isequal(const vector4& v, float tol) const
{
    if (n_fabs(v.x - x) > tol)      return false;
    else if (n_fabs(v.y - y) > tol) return false;
    else if (n_fabs(v.z - z) > tol) return false;
    else if (n_fabs(v.w - w) > tol) return false;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
vector4::compare(const vector4& v, float tol) const
{
    if (n_fabs(v.x - x) > tol)      return (v.x > x) ? +1 : -1;
    else if (n_fabs(v.y - y) > tol) return (v.y > y) ? +1 : -1;
    else if (n_fabs(v.z - z) > tol) return (v.z > z) ? +1 : -1;
    else if (n_fabs(v.w - w) > tol) return (v.w > w) ? +1 : -1;
    else                          return 0;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::minimum(const vector4& v)
{
    if (v.x < x) x = v.x;
    if (v.y < y) y = v.y;
    if (v.z < z) z = v.z;
    if (v.w < w) w = v.w;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::maximum(const vector4& v)
{
    if (v.x > x) x = v.x;
    if (v.y > y) y = v.y;
    if (v.z > z) z = v.z;
    if (v.w > w) w = v.w;
}

//------------------------------------------------------------------------------
/**
*/
static
inline
vector4 operator +(const vector4& v0, const vector4& v1)
{
    return vector4(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z, v0.w + v1.w);
}

//------------------------------------------------------------------------------
/**
*/
static
inline
vector4 operator -(const vector4& v0, const vector4& v1)
{
    return vector4(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z, v0.w - v1.w);
}

//------------------------------------------------------------------------------
/**
*/
static
inline
vector4 operator *(const vector4& v0, const float& s)
{
    return vector4(v0.x * s, v0.y * s, v0.z * s, v0.w * s);
}

//------------------------------------------------------------------------------
/**
*/
static
inline
vector4 operator -(const vector4& v)
{
    return vector4(-v.x, -v.y, -v.z, -v.w);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::setcomp(float val, int mask)
{
    if (mask & X) x = val;
    if (mask & Y) y = val;
    if (mask & Z) z = val;
    if (mask & W) w = val;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
vector4::getcomp(int mask)
{
    switch (mask)
    {
        case X:  return x;
        case Y:  return y;
        case Z:  return z;
        default: return w;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
int
vector4::mincompmask() const
{
    float minVal = x;
    int minComp = X;
    if (y < minVal)
    {
        minComp = Y;
        minVal  = y;
    }
    if (z < minVal)
    {
        minComp = Z;
        minVal  = z;
    }
    if (w < minVal)
    {
        minComp = W;
        minVal  = w;
    }
    return minComp;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::lerp(const vector4& v0, float lerpVal)
{
    x = v0.x + ((x - v0.x) * lerpVal);
    y = v0.y + ((y - v0.y) * lerpVal);
    z = v0.z + ((z - v0.z) * lerpVal);
    w = v0.w + ((w - v0.w) * lerpVal);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::lerp(const vector4& v0, const vector4& v1, float lerpVal)
{
    x = v0.x + ((v1.x - v0.x) * lerpVal);
    y = v0.y + ((v1.y - v0.y) * lerpVal);
    z = v0.z + ((v1.z - v0.z) * lerpVal);
    w = v0.w + ((v1.w - v0.w) * lerpVal);
}


//------------------------------------------------------------------------------
/**
*/
inline
void
vector4::saturate()
{
    x = n_saturate(x);
    y = n_saturate(y);
    z = n_saturate(z);
    w = n_saturate(w);
}

//------------------------------------------------------------------------------
/**
    Dot product for vector4
*/
inline
float vector4::dot(const vector4& v0) const
{
    return (x * v0.x + y * v0.y + z * v0.z + w * v0.w);
}

//------------------------------------------------------------------------------
#endif

