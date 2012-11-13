#ifndef N_SPHERE_H
#define N_SPHERE_H
//------------------------------------------------------------------------------
/**
    @class sphere
    @ingroup NebulaMathDataTypes

    A 3-dimensional sphere.

    (C) 2004 RadonLabs GmbH
*/
#include "mathlib/vector.h"
#include "mathlib/bbox.h"
#include "mathlib/matrix.h"
#include "mathlib/rectangle.h"

//------------------------------------------------------------------------------
class sphere
{
public:

    /// default constructor
    sphere();
    /// pos/radius constructor
    sphere(const vector3& _p, float _r);
    /// x,y,z,r constructor
    sphere(float _x, float _y, float _z, float _r);
    /// copy constructor
    sphere(const sphere& rhs);
    /// set position and radius
    void set(const vector3& _p, float _r);
    /// set x,y,z, radius
    void set(float _x, float _y, float _z, float _r);
    /// return true if box is completely inside sphere
    bool inside(const bbox3& box) const;
    /// check if 2 spheres overlap
    bool intersects(const sphere& s) const;
    /// check if sphere intersects box
    bool intersects(const bbox3& box) const;
    /// check if 2 moving sphere have contact
    bool intersect_sweep(const vector3& va, const sphere& sb, const vector3& vb, float& u0, float& u1) const;
    /// project sphere to screen rectangle (right handed coordinate system)
    rectangle project_screen_rh(const matrix44& modelView, const matrix44& projection, float nearZ) const;
    /// get clip status of box against sphere
    EClipStatus clipstatus(const bbox3& box) const;

    vector3 p;  // position
    float   r;  // radius
};

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere() :
    r(1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere(const vector3& _p, float _r) :
    p(_p),
    r(_r)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere(float _x, float _y, float _z, float _r) :
    p(_x, _y, _z),
    r(_r)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere(const sphere& rhs) :
    p(rhs.p),
    r(rhs.r)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
sphere::set(const vector3& _p, float _r)
{
    this->p = _p;
    this->r = _r;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
sphere::set(float _x, float _y, float _z, float _r)
{
    this->p.set(_x, _y, _z);
    this->r = _r;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
sphere::intersects(const sphere& s) const
{
    vector3 d(s.p - p);
    float rsum = s.r + r;
    if (d.lensquared() <= rsum * rsum) return true;
    else                               return false;
}

//------------------------------------------------------------------------------
/**
    Return true if the bounding box is inside the sphere.
*/
inline
bool
sphere::inside(const bbox3& box) const
{
    if (((this->p.x - r) < box.vmin.x) &&
        ((this->p.x + r) > box.vmax.x) &&
        ((this->p.y - r) < box.vmin.y) &&
        ((this->p.y + r) > box.vmax.y) &&
        ((this->p.z - r) < box.vmin.z) &&
        ((this->p.z + r) > box.vmax.z))
    {
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Check if sphere intersects with box.
    Taken from "Simple Intersection Tests For Games",
    Gamasutra, Oct 18 1999
*/
inline
bool
sphere::intersects(const bbox3& box) const
{
    float s, d = 0;

    // find the square of the distance
    // from the sphere to the box,
    if (p.x < box.vmin.x)
    {
        s = p.x - box.vmin.x;
        d += s*s;
    }
    else if (p.x > box.vmax.x)
    {
        s = p.x - box.vmax.x;
        d += s*s;
    }

    if (p.y < box.vmin.y)
    {
        s = p.y - box.vmin.y;
        d += s*s;
    }
    else if (p.y > box.vmax.y)
    {
        s = p.y - box.vmax.y;
        d += s*s;
    }

    if (p.z < box.vmin.z)
    {
        s = p.z - box.vmin.z;
        d += s*s;
    }
    else if (p.z > box.vmax.z)
    {
        s = p.z - box.vmax.z;
        d += s*s;
    }

    return d <= r*r;
}

//------------------------------------------------------------------------------
/**
    Get the clip status of a box against this sphere. Inside means: the
    box is completely inside the sphere.
*/
inline
EClipStatus
sphere::clipstatus(const bbox3& box) const
{
    if (this->inside(box)) return Inside;
    if (this->intersects(box)) return Clipped;
    return Outside;
}

//------------------------------------------------------------------------------
/**
    Check if 2 moving spheres have contact.
    Taken from "Simple Intersection Tests For Games"
    article in Gamasutra, Oct 18 1999

    @param  va  [in] distance traveled by 'this'
    @param  sb  [in] the other sphere
    @param  vb  [in] distance traveled by sb
    @param  u0  [out] normalized intro contact
    @param  u1  [out] normalized outro contact
*/
inline
bool
sphere::intersect_sweep(const vector3& va, const sphere&  sb, const vector3& vb, float& u0, float& u1) const
{
    vector3 vab(vb - va);
    vector3 ab(sb.p - p);
    float rab = r + sb.r;

    // check if spheres are currently overlapping...
    if ((ab % ab) <= (rab * rab))
    {
        u0 = 0.0f;
        u1 = 0.0f;
        return true;
    }
    // check if they hit each other
    float a = vab % vab;
    if ((a < -TINY) || (a > TINY))
    {
        // if a is '0' then the objects don't move relative to each other
        float b = (vab % ab) * 2.0f;
        float c = (ab % ab) - (rab * rab);
        float q = b*b - 4*a*c;
        if (q >= 0.0f)
        {
            // 1 or 2 contacts
            float sq = (float) sqrt(q);
            float d  = 1.0f / (2.0f*a);
            float r1 = (-b + sq) * d;
            float r2 = (-b - sq) * d;
            if (r1 < r2)
            {
                u0 = r1;
                u1 = r2;
            }
            else
            {
                u0 = r2;
                u1 = r1;
            }
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Project the sphere (defined in global space) to a screen space rectangle,
    given the current View and Projection matrices. The method assumes that
    the sphere is at least partially visible.
*/
inline
rectangle
sphere::project_screen_rh(const matrix44& view, const matrix44& projection, float nearZ) const
{
    // compute center point of the sphere in view space
    vector3 viewPos = view * this->p;
    if (viewPos.z > -nearZ)
    {
        viewPos.z = -nearZ;
    }
    vector3 screenPos  = projection.mult_divw(viewPos);
    screenPos.y = -screenPos.y;

    // compute size of sphere at its front size
    float frontZ = viewPos.z + this->r;
    if (frontZ > -nearZ)
    {
        frontZ = -nearZ;
    }
    vector3 screenSize = projection.mult_divw(vector3(this->r, this->r, frontZ));
    screenSize.y = -screenSize.y;
    float left   = n_saturate(0.5f * (1.0f + (screenPos.x - screenSize.x)));
    float right  = n_saturate(0.5f * (1.0f + (screenPos.x + screenSize.x)));
    float top    = n_saturate(0.5f * (1.0f + (screenPos.y + screenSize.y)));
    float bottom = n_saturate(0.5f * (1.0f + (screenPos.y - screenSize.y)));

    return rectangle(vector2(left, top), vector2(right, bottom));
}
//------------------------------------------------------------------------------
#endif
