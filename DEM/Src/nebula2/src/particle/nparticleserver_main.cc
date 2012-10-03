//------------------------------------------------------------------------------
//  nparticleserver_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "particle/nparticleserver.h"
#include <stdlib.h>
#include <time.h>

nParticleServer* nParticleServer::Singleton = 0;

//------------------------------------------------------------------------------
/**
*/
nParticleServer::nParticleServer() :
    enabled(true),
    particlePool(MaxParticles, 0, nParticle()),
    freeParticlePool(0, MaxParticles, 0),
    emitterPool(0, 10),
    floatRandomPool(FloatRandomCount, 0, 0.0f),
    intRandomPool(IntRandomCount, 0, 0),
    globalAccel(0.0f, -1.0f, 0.0f)
{
    n_assert(0 == Singleton);
    Singleton = this;

    ParticlePool::iterator particleIter = particlePool.Begin();
    while (particleIter != particlePool.End())
    {
        // store pointers to the free particles in the free particle pool
        freeParticlePool.PushBack(particleIter);
        ++particleIter;
    }

    srand((unsigned int) time(NULL));

    IntRandomPool::iterator intRandomIter = this->intRandomPool.Begin();
    while (intRandomIter != this->intRandomPool.End())
    {
        *intRandomIter = rand();
        intRandomIter ++;
     }

    FloatRandomPool::iterator floatRandomIter = this->floatRandomPool.Begin();
    while (floatRandomIter != this->floatRandomPool.End())
    {
        // get a random number between -1.0f and 1.0f
        float f0 = (2.0f*((float)rand())/((float)RAND_MAX))-1.0f;
        float f1 = (2.0f*((float)rand())/((float)RAND_MAX))-1.0f;
        float f2 = (2.0f*((float)rand())/((float)RAND_MAX))-1.0f;
        float f3 = (2.0f*((float)rand())/((float)RAND_MAX))-1.0f;

        float l = n_sqrt(f0*f0 + f1*f1 + f2*f2);
        if (l > 0.0f)
        {
            f0/=l; f1/=l; f2/=l;
        }

        *floatRandomIter = f0;
        floatRandomIter++;
        *floatRandomIter = f1;
        floatRandomIter++;
        *floatRandomIter = f2;
        floatRandomIter++;
        *floatRandomIter = f3;
        floatRandomIter++;
     }
}

//------------------------------------------------------------------------------
/**
*/
nParticleServer::~nParticleServer()
{
    EmitterPool::iterator emitterIter = this->emitterPool.Begin();
    while (emitterIter != this->emitterPool.End())
    {
        n_delete(*emitterIter);
        ++emitterIter;
    }
    n_assert(Singleton);
    Singleton = 0;
}


//------------------------------------------------------------------------------
/**
*/
nParticleEmitter*
nParticleServer::GetParticleEmitter(int key)
{
    nParticleEmitter* emitter = NULL;

    if (!this->emitterPool.Empty())
    {
        for (EmitterPool::iterator emitterIter = this->emitterPool.Begin();
            emitterIter != this->emitterPool.End(); ++emitterIter)
        {
            if ((*emitterIter)->GetKey() == key)
            {
                emitter = *emitterIter;
                break;
            }
        }
    }
    return emitter;
}


//------------------------------------------------------------------------------
/**
*/
nParticleEmitter*
nParticleServer::NewParticleEmitter()
{
    nParticleEmitter* particleEmitter = n_new(nParticleEmitter);
    this->emitterPool.Append(particleEmitter);
    // n_printf("nParticleServer: particle emitter created!\n");
    return particleEmitter;
}

//------------------------------------------------------------------------------
/**
    - 13-Feb-04 floh    triggering emitters now in nParticleEmitter::Trigger()

*/
void nParticleServer::Trigger()
{
    // Trigger active emitters and delete unused
    if (!this->emitterPool.Empty())
    {
        EmitterPool::iterator emitterIter = this->emitterPool.Begin();
        while (emitterIter != this->emitterPool.End())
        {
            nParticleEmitter* emitter = *emitterIter;
            if (!emitter->IsAlive() || emitter->GetFatalException())
            {
                n_delete(emitter);
                // n_printf("nParticleServer: Deleting particle emitter!\n");
                emitterIter = emitterPool.EraseQuick(emitterIter);
            }
            else
            {
                // FIXME: now in nParticleEmitter::Trigger()
                emitter->SetAlive(false);
                ++emitterIter;
            }
        }
    }
}

