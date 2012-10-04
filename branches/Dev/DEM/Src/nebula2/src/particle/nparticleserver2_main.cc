//------------------------------------------------------------------------------
//  nparticleserver_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "particle/nparticleserver2.h"
#include <stdlib.h>
#include <time.h>

nParticleServer2* nParticleServer2::Singleton = 0;

//------------------------------------------------------------------------------
/**
*/
nParticleServer2::nParticleServer2() :
    floatRandomPool(FloatRandomCount),
    intRandomPool(IntRandomCount),
    globalAccel(0.0f, -1.0f, 0.0f),
    time(0.0)
{
    n_assert(0 == Singleton);
    Singleton = this;

    srand((unsigned int) ::time(NULL));

    // fill the random number pools
    int i;
    for (i = 0; i < this->intRandomPool.Size(); i++)
    {
        this->intRandomPool[i] = rand();
    }
    for (i = 0; i < this->floatRandomPool.Size();)
    {
        // get a random number between -1.0f and 1.0f, organized
        // into 3 normalized vector components, and one w component
        vector3 v(n_rand(-1.0f, 1.0f), n_rand(-1.0f, 1.0f), n_rand(-1.0f, 1.0f));
        float w = n_rand(-1.0f, 1.0f);
        v.norm();
        this->floatRandomPool[i++] = v.x;
        this->floatRandomPool[i++] = v.y;
        this->floatRandomPool[i++] = v.z;
        this->floatRandomPool[i++] = w;
    }
}

//------------------------------------------------------------------------------
/**
*/
nParticleServer2::~nParticleServer2()
{
    n_assert(0 == emitters.Size());
    n_assert(Singleton);
    Singleton = 0;
}


//------------------------------------------------------------------------------
/**
*/
nParticle2Emitter*
nParticleServer2::NewParticleEmitter()
{
    nParticle2Emitter* particleEmitter = n_new(nParticle2Emitter);
    this->emitters.PushBack(particleEmitter);
    n_printf("nParticleServer2: particle emitter created!\n");
    return particleEmitter;
}

//------------------------------------------------------------------------------
/**
*/
void nParticleServer2::Trigger()
{
    this->time = TimeSrv->GetTime();
    
    int num = this->emitters.Size();
    for (int i = 0; i < num; i++)
    {
        this->emitters[i]->Update(float(this->time));
    }
}

//------------------------------------------------------------------------------
/**
*/
void nParticleServer2::DeleteParticleEmitter(nParticle2Emitter* emitter)
{
    n_assert(0 != emitter);
    int index = this->emitters.FindIndex(emitter);
    if (-1 != index)
    {
        n_delete(this->emitters[index]);
        this->emitters.Erase(index);
    }
}
