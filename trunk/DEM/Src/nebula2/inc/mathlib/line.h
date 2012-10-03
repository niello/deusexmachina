#ifndef N_LINE_H
#define N_LINE_H
//-------------------------------------------------------------------
/**
    @class line2
    @ingroup NebulaMathDataTypes

    A 2-dimensional line class.

    (C) 2004 RadonLabs GmbH
*/
#include "mathlib/vector.h"

//-------------------------------------------------------------------
class line2
{
public:
    /// constructor
    line2();
    /// constructor #2
    line2(const vector2& v0, const vector2& v1);
    /// copy constructor
    line2(const line2& rhs);
    /// return start point
    const vector2& start() const;
    /// return end point
    vector2 end() const;
    /// return vector
    const vector2& vec() const;
    /// return length
    float len() const;
    /// get point on line given t
    vector2 ipol(const float t) const;

    /// point of origin
    vector2 b;
    /// direction
    vector2 m;
};

//-------------------------------------------------------------------
/**
*/
inline
line2::line2()
{
    // empty
}

//-------------------------------------------------------------------
/**
*/
inline
line2::line2(const vector2& v0, const vector2& v1) :
    b(v0),
    m(v1 - v0)
{
    // empty
}

//-------------------------------------------------------------------
/**
*/
inline
line2::line2(const line2& rhs) :
    b(rhs.b),
    m(rhs.m)
{
    // empty
}

//-------------------------------------------------------------------
/**
*/
inline
const vector2&
line2::start() const
{
    return this->b;
}

//-------------------------------------------------------------------
/**
*/
inline
vector2
line2::end() const
{
    return this->b + this->m;
}

//-------------------------------------------------------------------
/**
*/
inline
const vector2&
line2::vec() const
{
    return this->m;
}

//-------------------------------------------------------------------
/**
*/
inline
float
line2::len() const
{
    return m.len();
}

//-------------------------------------------------------------------
/**
*/
inline
vector2
line2::ipol(const float t) const
{
    return vector2(b + m * t);
}

//------------------------------------------------------------------------------
/**
    @class line3
    @ingroup NebulaMathDataTypes

    A 3-dimensional line class.
*/
class line3
{
public:
    /// constructor
    line3();
    /// constructor #2
    line3(const vector3& v0, const vector3& v1);
    /// copy constructor
    line3(const line3& l);
    /// set start and end point
    void set(const vector3& v0, const vector3& v1);
    /// get start point
    const vector3& start() const;
    /// get end point
    vector3 end() const;
    /// get vector
    const vector3& vec() const;
    /// get length
    float len() const;
    /// get squared length
    float lensquared() const;
    /// minimal distance of point to line
    float distance(const vector3& p) const;
    /// get point on line at t
    vector3 ipol(float t) const;
    /// intersect
    bool intersect(const line3& l, vector3& pa, vector3& pb) const;
    /// return t of the closest point on the line
    float closestpoint(const vector3& p) const;
    /// return p = b + m*t
    vector3 point(float t) const;

    /// point of origin
    vector3 b;
    /// direction
    vector3 m;
};

//------------------------------------------------------------------------------
/**
*/
inline
line3::line3()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
line3::line3(const vector3& v0, const vector3& v1) :
    b(v0),
    m(v1 - v0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
line3::line3(const line3& rhs) :
    b(rhs.b),
    m(rhs.m)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
line3::set(const vector3& v0, const vector3& v1)
{
    this->b = v0;
    this->m = v1 - v0;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
line3::start() const
{
    return this->b;
}

//------------------------------------------------------------------------------
/**
*/
inline
vector3
line3::end() const
{
    return this->b + this->m;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
line3::vec() const
{
    return this->m;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
line3::len() const
{
    return this->m.len();
}

//------------------------------------------------------------------------------
/**
*/
inline
float
line3::lensquared() const
{
    return this->m.lensquared();
}

//------------------------------------------------------------------------------
/**
    Returns a point on the line which is closest to a another point in space.
    This just returns the parameter t on where the point is located. If t is
    between 0 and 1, the point is on the line, otherwise not. To get the
    actual 3d point p:

    p = m + b*t
*/
inline
float
line3::closestpoint(const vector3& p) const
{
    vector3 diff(p - this->b);
    float l = (this->m % this->m);
    if (l > 0.0f)
    {
        float t = (this->m % diff) / l;
        return t;
    }
    else
    {
        return 0.0f;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
float
line3::distance(const vector3& p) const
{
    vector3 diff(p - this->b);
    float l = (this->m % this->m);
    if (l > 0.0f)
    {
        float t = (this->m % diff) / l;
        diff = diff - this->m * t;
        return diff.len();
    }
    else
    {
        // line is really a point...
        vector3 v(p - this->b);
        return v.len();
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
vector3
line3::ipol(const float t) const
{
    return vector3(b + m*t);
}

//------------------------------------------------------------------------------
/**
    Get line/line intersection. Returns the shortest line between two lines.
*/
inline
bool
line3::intersect(const line3& l, vector3& pa, vector3& pb) const
{
   const float EPS = 2.22e-16f;
   vector3 p1 = this->b;
   vector3 p2 = this->ipol(10.0f);
   vector3 p3 = l.b;
   vector3 p4 = l.ipol(10.0f);
   vector3 p13, p43, p21;
   float d1343,d4321,d1321,d4343,d2121;
   float numer, denom;
   float mua, mub;

   p13.x = p1.x - p3.x;
   p13.y = p1.y - p3.y;
   p13.z = p1.z - p3.z;
   p43.x = p4.x - p3.x;
   p43.y = p4.y - p3.y;
   p43.z = p4.z - p3.z;
   if (n_abs(p43.x) < EPS && n_abs(p43.y) < EPS && n_abs(p43.z) < EPS) return false;
   p21.x = p2.x - p1.x;
   p21.y = p2.y - p1.y;
   p21.z = p2.z - p1.z;
   if (n_abs(p21.x)  < EPS && n_abs(p21.y) < EPS && n_abs(p21.z) < EPS) return false;

   d1343 = p13.x * p43.x + p13.y * p43.y + p13.z * p43.z;
   d4321 = p43.x * p21.x + p43.y * p21.y + p43.z * p21.z;
   d1321 = p13.x * p21.x + p13.y * p21.y + p13.z * p21.z;
   d4343 = p43.x * p43.x + p43.y * p43.y + p43.z * p43.z;
   d2121 = p21.x * p21.x + p21.y * p21.y + p21.z * p21.z;

   denom = d2121 * d4343 - d4321 * d4321;
   if (n_abs(denom) < EPS) return false;
   numer = d1343 * d4321 - d1321 * d4343;

   mua = numer / denom;
   mub = (d1343 + d4321 * (mua)) / d4343;

   pa.x = p1.x + mua * p21.x;
   pa.y = p1.y + mua * p21.y;
   pa.z = p1.z + mua * p21.z;
   pb.x = p3.x + mub * p43.x;
   pb.y = p3.y + mub * p43.y;
   pb.z = p3.z + mub * p43.z;

   return true;
}

//------------------------------------------------------------------------------
/**
    Returns p = b + m * t, given t. Note that the point is not on the line
    if 0.0 > t > 1.0
*/
inline
vector3
line3::point(float t) const
{
    return this->b + this->m * t;
}
//------------------------------------------------------------------------------
#endif
