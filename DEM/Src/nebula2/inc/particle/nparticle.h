#ifndef N_PARTICLE_H
#define N_PARTICLE_H
//------------------------------------------------------------------------------
/**
    @class nParticle
    @ingroup NebulaParticleSystem
    @brief This class represents a single particle in a particle system.

    (C) 2003 RadonLabs GmbH
*/

#include "mathlib/vector.h"
#include "mathlib/matrix.h"

class nParticleEmitter;

//------------------------------------------------------------------------------
class nParticle
{
public:
    enum State
    {
        Unborn,
        Living,
        Dead
    };

    /// constructor
    nParticle();
    /// destructor
    ~nParticle();
    /// initializes all values and sets state to Used
    void Initialize(const vector3& position, const vector3& velocity, float birthTime, float lifeTime, float rotation);
    /// update the particle
    void Trigger(nParticleEmitter* emitter, float curTime, float frameTime, const vector3& absAccel);
    /// get the current particle position
    const vector3& GetPosition() const;
    /// get the current particle velocity
    const vector3& GetVelocity() const;
    /// get the current particle rotation
    float GetRotation();
    /// set the birth time of the particle
    void SetBirthTime(float time);
    /// get birth time of particle
    float GetBirthTime() const;
    /// set the life time of the particle
    void SetLifeTime(float time);
    /// get life time of particle
    float GetLifeTime() const;
    /// set the state of the particle
    void SetState(State state);
    /// get the state of the particle
    State GetState() const;
    /// returns the age of the particle
    // nTime GetAge() const;
    /// computes the current relative age of the particle, clamped to 0.0 -> 1.0
    float GetRelativeAge(float curTime) const;

protected:
    vector3 velocity;
    vector3 position;
    State state;
    float birthTime;
    float lifeTime;
    float rotation;
};

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle::SetState(State state)
{
    this->state = state;
}

//------------------------------------------------------------------------------
/**
*/
inline
nParticle::State
nParticle::GetState() const
{
    return this->state;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle::SetBirthTime(float time)
{
    this->birthTime = time;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nParticle::GetBirthTime() const
{
    return this->birthTime;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle::SetLifeTime(float time)
{
    this->lifeTime = time;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nParticle::GetLifeTime() const
{
    return this->lifeTime;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nParticle::GetPosition() const
{
    return this->position;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nParticle::GetVelocity() const
{
    return this->velocity;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nParticle::GetRotation()
{
    return this->rotation;
}

//------------------------------------------------------------------------------
/**
*/
/*
inline
nTime nParticle::GetAge() const
{
    return (this->lastTrigger - this->birthTime);
}
*/

//------------------------------------------------------------------------------
/**
*/
inline
float
nParticle::GetRelativeAge(float curTime) const
{
    return n_saturate(((curTime - this->birthTime) / this->lifeTime));
}

//------------------------------------------------------------------------------
#endif

