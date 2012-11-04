#ifndef N_LIGHT_H
#define N_LIGHT_H
//------------------------------------------------------------------------------
/**
    @class nLight
    @ingroup Gfx2

    Describes a light source.

    (C) 2004 RadonLabs GmbH
*/
#include "kernel/ntypes.h"
#include "mathlib/matrix.h"

//------------------------------------------------------------------------------
class nLight
{
public:

	enum Type
	{
		Point		= 0,
		Directional	= 1,
		Spot		= 2
	};

private:

	Type type;
    float range;
    vector4 diffuse;
    vector4 specular;
    vector4 ambient;
    vector4 shadowLightMask;
    bool castShadows;

public:

    /// constructor
    nLight();

	/// set light type
    void SetType(Type t);
    /// get light type
    Type GetType() const;
    /// set light range
    void SetRange(float r);
    /// get light range
    float GetRange() const;
    /// set light's diffuse color
    void SetDiffuse(const vector4& c);
    /// get light's diffuse color
    const vector4& GetDiffuse() const;
    /// set light's specular color
    void SetSpecular(const vector4& c);
    /// get light's specular color
    const vector4& GetSpecular() const;
    /// set light's ambient color
    void SetAmbient(const vector4& c);
    /// get light's ambient color
    const vector4& GetAmbient() const;
    /// set cast shadows
    void SetCastShadows(const bool& b);
    /// set cast shadows
    const bool& GetCastShadows() const;
    /// set light mask for shadow rendering
    void SetShadowLightMask(const vector4& m);
    /// get light mask for shadow rendering
    const vector4& GetShadowLightMask() const;
    /// convert type string to enum
    static Type StringToType(const char* str);
    /// convert type enum to string
    static const char* TypeToString(Type t);
};

//------------------------------------------------------------------------------
/**
*/
inline
nLight::nLight() :
    type(Point),
    range(1.0f),
    diffuse(1.0f, 1.0f, 1.0f, 1.0f),
    specular(1.0f, 1.0f, 1.0f, 1.0f),
    ambient(0.0f, 0.0f, 0.0f, 0.0f),
    castShadows(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLight::SetRange(float r)
{
    this->range = r;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nLight::GetRange() const
{
    return this->range;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLight::SetType(Type t)
{
    this->type = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nLight::Type
nLight::GetType() const
{
    return this->type;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLight::SetDiffuse(const vector4& c)
{
    this->diffuse = c;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nLight::GetDiffuse() const
{
    return this->diffuse;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLight::SetSpecular(const vector4& c)
{
    this->specular = c;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nLight::GetSpecular() const
{
    return this->specular;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLight::SetAmbient(const vector4& c)
{
    this->ambient = c;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nLight::GetAmbient() const
{
    return this->ambient;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLight::SetCastShadows(const bool& b)
{
    this->castShadows = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
const bool&
nLight::GetCastShadows() const
{
    return this->castShadows;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nLight::SetShadowLightMask(const vector4& m)
{
    this->shadowLightMask = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector4&
nLight::GetShadowLightMask() const
{
    return this->shadowLightMask;
}
//------------------------------------------------------------------------------
/**
*/
inline
const char*
nLight::TypeToString(Type t)
{
    switch (t)
    {
        case Point:         return "Point";
        case Directional:   return "Directional";
        case Spot:          return "Spot";
        default:
            n_error("nLight::TypeToString(): invalid light type value '%d'!", t);
            return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
nLight::Type
nLight::StringToType(const char* str)
{
    n_assert(str);
    if (0 == strcmp(str, "Point")) return Point;
    if (0 == strcmp(str, "Directional")) return Directional;
    if (0 == strcmp(str, "Spot")) return Spot;
    n_error("nLight::StringToType(): invalid light type string '%s'!", str);
    return Point;
}

//------------------------------------------------------------------------------
#endif
