#ifndef N_RECTANGLE_H
#define N_RECTANGLE_H
//------------------------------------------------------------------------------
/**
    @class rectangle
    @ingroup NebulaMathDataTypes

    A 2d rectangle class.

    (C) 2003 RadonLabs GmbH
*/
#include "Math/Vector2.h"

//------------------------------------------------------------------------------
class rectangle
{
public:
    /// default constructor
    rectangle();
    /// constructor 1
    rectangle(const vector2& topLeft, const vector2& bottomRight);
    /// set content
    void set(const vector2& topLeft, const vector2& bottomRight);
    /// return true if point is inside
    bool inside(const vector2& p) const;
    /// return midpoint
    vector2 midpoint() const;
    /// return width
    float width() const;
    /// return height
    float height() const;
    /// return (width * height)
    float area() const;
    /// return size
    vector2 size() const;

    vector2 v0;
    vector2 v1;
};

//------------------------------------------------------------------------------
/**
*/
inline
rectangle::rectangle()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
rectangle::rectangle(const vector2& topLeft, const vector2& bottomRight) :
    v0(topLeft),
    v1(bottomRight)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
rectangle::set(const vector2& topLeft, const vector2& bottomRight)
{
    this->v0 = topLeft;
    this->v1 = bottomRight;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
rectangle::inside(const vector2& p) const
{
    return ((this->v0.x <= p.x) && (p.x <= this->v1.x) &&
            (this->v0.y <= p.y) && (p.y <= this->v1.y));
}

//------------------------------------------------------------------------------
/**
*/
inline
vector2
rectangle::midpoint() const
{
    return (this->v0 + this->v1) * 0.5f;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
rectangle::width() const
{
    return this->v1.x - this->v0.x;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
rectangle::height() const
{
    return this->v1.y - this->v0.y;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
rectangle::area() const
{
    return this->width() * this->height();
}

//------------------------------------------------------------------------------
/**
*/
inline
vector2
rectangle::size() const
{
    return this->v1 - this->v0;
}

//------------------------------------------------------------------------------
/**
*/
static
inline
rectangle operator *(const rectangle& r0, const rectangle& r1)
{
    float v0x, v0y, v1x, v1y;

    if (r0.v0.x >= r1.v1.x || r0.v1.x <= r1.v0.x ||
        r0.v0.y >= r1.v1.y || r0.v1.y <= r1.v0.y ||
        r0.area() == 0.0f || r1.area() == 0.0f)
    {
        // zero-size rectangle
        v0x = v1x = r0.v0.x;
        v0y = v1y = r0.v0.y;
    }
    else
    {
        v0x = n_max(r0.v0.x, r1.v0.x);
        v0y = n_max(r0.v0.y, r1.v0.y);
        v1x = n_min(r0.v1.x, r1.v1.x);
        v1y = n_min(r0.v1.y, r1.v1.y);
    }

    return rectangle(vector2(v0x, v0y), vector2(v1x, v1y));
}
//------------------------------------------------------------------------------
#endif
