#ifndef N_PARTICLESERVER_H
#define N_PARTICLESERVER_H
//------------------------------------------------------------------------------
/**
    @class nParticleServer
    @ingroup NebulaParticleSystem
    @brief
    Particle subsystem server. Holds an array of all particles and all particle
    emitters in the world. Take care when updating them, the rendering is a
    function of the emitters.

    (C) 2003 RadonLabs GmbH
*/
#include "kernel/nroot.h"
#include "particle/nparticle.h"
#include "particle/nparticleemitter.h"
#include "util/narray.h"
#include "util/nringbuffer.h"

//------------------------------------------------------------------------------
class nParticle;
class nParticleEmitter;

class nParticleServer: public nReferenced
{
private:
    typedef nArray<nParticle> ParticlePool;
    typedef nArray<nParticle*> FreeParticlePool;
    typedef nArray<nParticleEmitter*> EmitterPool;
    typedef nArray<float> FloatRandomPool;
    typedef nArray<int> IntRandomPool;

    enum
    {
        MaxParticles = 10000,       // maximum number of particles in the world
        FloatRandomCount = 65536,   // number of floats in the float random pool
        IntRandomCount = 512,       // number of ints in the int random pool
    };
public:
    /// constructor
    nParticleServer();
    /// destructor
    virtual ~nParticleServer();
    /// get server instance
    static nParticleServer* Instance();
    /// enable/disable particle subsystem
    void SetEnabled(bool b);
    /// is currently enabled?
    bool IsEnabled() const;
    /// Update particles and emitters, delete unused emitters
    void Trigger();
    /// get particle emitter by its key
    nParticleEmitter* GetParticleEmitter(int key);
    /// create a new particle emitter
    nParticleEmitter* NewParticleEmitter();
    /// get a free particle from the pool, or 0 if none exists
    nParticle* GiveFreeParticle();
    /// a particle can go back to the free pool
    void TakeBackParticle(nParticle* particle);
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
    static nParticleServer* Singleton;

    bool enabled;
    EmitterPool        emitterPool;         // stores all used emitters
    ParticlePool       particlePool;        // stores all particles in the world
    FreeParticlePool   freeParticlePool;    // points to all free particles in particlePool
    FloatRandomPool    floatRandomPool;
    IntRandomPool      intRandomPool;
    vector3            globalAccel;
};

//------------------------------------------------------------------------------
/**
*/
inline
nParticleServer*
nParticleServer::Instance()
{
    n_assert(Singleton);
    return Singleton;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleServer::SetEnabled(bool b)
{
    this->enabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticleServer::IsEnabled() const
{
    return this->enabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleServer::SetGlobalAccel(const vector3& accel)
{
    this->globalAccel = accel;
}

//------------------------------------------------------------------------------
/**
*/
inline
const vector3&
nParticleServer::GetGlobalAccel() const
{
    return this->globalAccel;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nParticleServer::PseudoRandomInt(int key)
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
nParticleServer::PseudoRandomFloat(int key)
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
nParticleServer::PseudoRandomVector3(int key)
{
    // align to start of random normalized 3d vector
    key *= 4;
    int k0 = key % (FloatRandomCount - 1);
    n_assert((k0 >= 0) && ((k0 + 2) < FloatRandomCount));
    return vector3(this->floatRandomPool[k0], this->floatRandomPool[k0+1], this->floatRandomPool[k0+2]);
};

//------------------------------------------------------------------------------
/**
*/
inline
nParticle*
nParticleServer::GiveFreeParticle()
{
    if (this->freeParticlePool.Empty())
    {
        return 0;
    }
    else
    {
        nParticle* particle = *(this->freeParticlePool.EraseQuick(this->freeParticlePool.End()-1));
        return particle;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleServer::TakeBackParticle(nParticle* particle)
{
    particle->SetState(nParticle::Unborn);
    this->freeParticlePool.PushBack(particle);
}

//------------------------------------------------------------------------------
#endif

