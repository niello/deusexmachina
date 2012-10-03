#ifndef N_ANIMEVENT_H
#define N_ANIMEVENT_H
//------------------------------------------------------------------------------
/**
    @class nAnimEvent
    @ingroup Anim2
    @brief An nAnimEvent holds time, translation, rotation and scale of an
    animation event.

    (C) 2005 Radon Labs GmbH
*/
#include "kernel/ntypes.h"
#include "mathlib/vector.h"
#include "mathlib/quaternion.h"
#include "mathlib/matrix.h"

//------------------------------------------------------------------------------
class nAnimEvent
{
public:
    /// constructor
    nAnimEvent();
    /// set time
    void SetTime(float t);
    /// get time
    float GetTime() const;
    /// set translation
    void SetTranslation(const vector3& t);
    /// get translation
    const vector3& GetTranslation() const;
    /// set rotation
    void SetQuaternion(const quaternion& q);
    /// get quaternion
    const quaternion& GetQuaternion() const;
    /// set scale
    void SetScale(const vector3& s);
    /// get scale
    const vector3& GetScale() const;
    /// get resulting matrix (slow)
    matrix44 GetMatrix() const;
    /// set accumulated weight
    void SetWeightAccum(float w);
    /// get accumulated weight
    float GetWeightAccum() const;

private:
    float time;
    vector3 translation;
    quaternion quat;
    vector3 scale;
    float weightAccum;      // helper value for quaternion blending
};

//------------------------------------------------------------------------------
/**
*/
inline
nAnimEvent::nAnimEvent() :
    time(0.0f),
    scale(1.0f, 1.0f, 1.0f),
    weightAccum(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEvent::SetTime(float t)
{
    this->time = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimEvent::GetTime() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEvent::SetTranslation(const vector3& t)
{
    this->translation = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nAnimEvent::GetTranslation() const
{
    return this->translation;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEvent::SetQuaternion(const quaternion& q)
{
    this->quat= q;
}

//------------------------------------------------------------------------------
/**
*/
inline
const quaternion&
nAnimEvent::GetQuaternion() const
{
    return this->quat;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEvent::SetScale(const vector3& s)
{
    this->scale = s;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nAnimEvent::GetScale() const
{
    return this->scale;
}

//------------------------------------------------------------------------------
/**
*/
inline
matrix44
nAnimEvent::GetMatrix() const
{
    matrix44 m;
    m.scale(this->scale);
    m.mult_simple(matrix44(this->quat));
    m.translate(this->translation);
    return m;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nAnimEvent::SetWeightAccum(float w)
{
    this->weightAccum = w;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nAnimEvent::GetWeightAccum() const
{
    return this->weightAccum;
}

//------------------------------------------------------------------------------
#endif
