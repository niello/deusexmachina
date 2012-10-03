#ifndef N_PARTICLE2_H
#define N_PARTICLE2_H
//------------------------------------------------------------------------------
/**
    @class nParticle2
    @ingroup Particle

    A single particle2 in a particle system
    just a typedef

    (C) 2005 RadonLabs GmbH
*/

#include "mathlib/vector.h"

typedef struct nParticle2
{
    vector3 acc;
    vector3 velocity;
    vector3 position;
    vector3 startPos;
    float   rotation;
    float   rotationVariation;
    float   sizeVariation;
    float   lifeTime;
    float   oneDivMaxLifeTime;
    vector2 uvmin;
    vector2 uvmax;
} nParticle2;

#endif

