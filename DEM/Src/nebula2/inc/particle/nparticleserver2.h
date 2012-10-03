#ifndef N_PARTICLESERVER2_H
#define N_PARTICLESERVER2_H
//------------------------------------------------------------------------------
/**
    @class nParticleServer2
    @ingroup Particle

    Particle subsystem server. Holds an array of all particles and all particle
    emitters in the world. Take care when updating them, the rendering is a
    function of the emitters.

    (C) 2005 RadonLabs GmbH
*/
#include "kernel/nroot.h"
#include "particle/nparticle2.h"
#include "particle/nparticle2emitter.h"
#include "util/nfixedarray.h"
#include "util/nringbuffer.h"

//------------------------------------------------------------------------------
class nParticle2Emitter;

class nParticleServer2: public nReferenced
{
private:
    enum
    {
        MaxParticles = 30000,       // maximum number of particles in the world
        FloatRandomCount = 32768,   // number of floats in the float random pool
        IntRandomCount = 512,       // number of ints in the int random pool
    };
public:
    /// constructor
    nParticleServer2();
    /// destructor
    virtual ~nParticleServer2();
    /// get server instance
    static nParticleServer2* Instance();
    /// update all active particle emitters
    void Trigger();

    /// set current time
    void SetTime(nTime t);
    /// get current time
    nTime GetTime() const;
    /// create a new particle emitter
    nParticle2Emitter* NewParticleEmitter();
    /// delete the given emitter
    void DeleteParticleEmitter(nParticle2Emitter* emitter);
    /// set global force attribute
    void SetGlobalAccel(const vector3& accel);
    /// get global force attribute
    const vector3& GetGlobalAccel() const;
    /// get a random int from the int random pool
    int PseudoRandomInt(int key);
    /// get a random float from the float random pool
    float PseudoRandomFloat(int key);
    /// pseudo random vector
    vector3 PseudoRandomVector3(int key);

private:
    static nParticleServer2* Singleton;

    nArray<nParticle2Emitter*> emitters;
    nFixedArray<float> floatRandomPool;
    nFixedArray<int> intRandomPool;
    vector3 globalAccel;
    nTime time;
};

//------------------------------------------------------------------------------
/**
*/
inline
nParticleServer2*
nParticleServer2::Instance()
{
    n_assert(Singleton);
    return Singleton;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleServer2::SetTime(nTime t)
{
    this->time = t;
}

//------------------------------------------------------------------------------
/**
*/
inline
nTime
nParticleServer2::GetTime() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleServer2::SetGlobalAccel(const vector3& accel)
{
    this->globalAccel = accel;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nParticleServer2::GetGlobalAccel() const
{
    return this->globalAccel;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nParticleServer2::PseudoRandomInt(int key)
{
    // force key into valid range
    key %= (IntRandomCount-1);
    n_assert(key >= 0);
    return this->intRandomPool[key];
};

//------------------------------------------------------------------------------
/**
*/
inline
float
nParticleServer2::PseudoRandomFloat(int key)
{
    // force key into valid range
    key %= (FloatRandomCount-1);
    n_assert(key >= 0);
    return this->floatRandomPool[key];
};

//------------------------------------------------------------------------------
/**
*/
inline
vector3
nParticleServer2::PseudoRandomVector3(int key)
{
    // align to start of random normalized 3d vector
    key *= 4;
    int k0 = key % (FloatRandomCount - 1);
    n_assert((k0 >= 0) && ((k0 + 2) < FloatRandomCount));
    return vector3(this->floatRandomPool[k0], this->floatRandomPool[k0+1], this->floatRandomPool[k0+2]);
};

//------------------------------------------------------------------------------
#endif

