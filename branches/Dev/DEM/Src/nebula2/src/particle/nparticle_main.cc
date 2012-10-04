//------------------------------------------------------------------------------
//  nparticle_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "particle/nparticle.h"
#include "particle/nparticleemitter.h"

//------------------------------------------------------------------------------
/**
*/
nParticle::nParticle() :
    state(Unborn),
    birthTime(0.0),
    lifeTime(0.0),
    rotation(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nParticle::~nParticle()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void nParticle::Initialize(const vector3& position, const vector3& velocity, float birthTime, float lifeTime, float rotation)
{
    this->position  = position;
    this->velocity  = velocity;
    this->rotation  = rotation;
    this->birthTime = birthTime;
    this->lifeTime  = lifeTime;
    this->state     = Unborn;
}

//------------------------------------------------------------------------------
/**
*/
void
nParticle::Trigger(nParticleEmitter* emitter, float curTime, float frameTime, const vector3& absAccel)
{
    static vector3 windVelocity;
    static vector3 finalVelocity;
    static const vector3 zVector(0.0, 0.0, 1.0);
    nTime curAge = curTime - this->birthTime;
    float relAge = (float) (curAge / lifeTime);

    switch (this->state)
    {
        case Unborn:
            if (curAge >= 0.0)
            {
                this->state = Living;
            }
            break;

        case Living:
            if (relAge >= 1.0)
            {
                this->state = Dead;
            }
            else
            {
                this->rotation += emitter->GetParticleRotationVelocity(relAge) * frameTime;

                // update velocity
                this->velocity += absAccel * emitter->GetParticleWeight(relAge) * frameTime;

                // absolute acceleration
                float airResistance = emitter->GetParticleAirResistance(relAge);
                const nFloat4& wind = emitter->GetWind();
                float windFactor = airResistance * wind.w;
                windVelocity.set(wind.x * windFactor, wind.y * windFactor, wind.z * windFactor);
                finalVelocity = (this->velocity * emitter->GetParticleVelocityFactor(relAge)) + windVelocity;
                this->position += finalVelocity * (float) frameTime;
            }
            break;

        default:
            break;
    }
}
