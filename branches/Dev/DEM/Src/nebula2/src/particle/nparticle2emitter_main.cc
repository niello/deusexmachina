//------------------------------------------------------------------------------
//  nparticle_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "particle/nparticle2emitter.h"
#include "particle/nparticle2.h"
#include "mathlib/vector.h"

//------------------------------------------------------------------------------
/**
*/
nParticle2Emitter::nParticle2Emitter() :
    pStaticCurves(0),
    emitterMeshGroupIndex(0),
    lastEmissionVertex(0),
    startTime(-1.0),
    lastEmission(0),
    emissionDuration(10.0),
    looping(true),
    activityDistance(100.0f),
    particleStretch(0),
    precalcTime(0),
    tileTexture(1),
    renderOldestFirst(false),
    stretchToStart(false),
    randomRotDir(false),
    hasLooped(false),
    frameWasRendered(true),
    invisibleTime(0),
    isSleeping(false),
    stretchDetail(1),
    viewAngleFade(false),
    startDelay(0.0f),
    emitOnSurface(false),
    gravity(0.0f),
    startRotationMin(0.0f),
    startRotationMax(0.0f),
    particleVelocityRandomize(0.0f),
    particleRotationRandomize(0.0f),
    particleSizeRandomize(0.0f),
    particles(0),
    particleCount(0),
    maxParticleCount(0),
    remainingTime(0.0),
    isSetup(false),
    isValid(false)
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
nParticle2Emitter::~nParticle2Emitter()
{
    this->DeleteParticles();
}

//------------------------------------------------------------------------------
/**
    Allocates the particle pool.
*/
void
nParticle2Emitter::AllocateParticles()
{
    n_assert(this->maxParticleCount > 0);
    this->DeleteParticles();
    this->particles = n_new_array(nParticle2, this->maxParticleCount);
    this->particleCount = 0;
}

//------------------------------------------------------------------------------
/**
    Deletes the particle pool.
*/
void
nParticle2Emitter::DeleteParticles()
{
    if (this->particles)
    {
        n_delete_array(this->particles);
        this->particles = 0;
        this->particleCount = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Updates the existing particles.
*/
void
nParticle2Emitter::CalculateStep(float stepTime)
{
    n_assert(stepTime >= 0.0f);
    n_assert(0 != this->particles);
    n_assert(0 != this->pStaticCurves);

    // nothing to do?
    if (0 == this->particleCount)
    {
        return;
    }

    vector3 windVector = vector3(this->wind.x, this->wind.y, this->wind.z) * wind.w;
    vector3 acc, freeAcc, boundMin, boundMax;
    this->box.begin_extend();

    nParticle2* particleSource = this->particles;
    nParticle2* particle = this->particles;
    int particleIndex;
    for (particleIndex = 0; particleIndex < this->particleCount; particleIndex++)
    {
        // FIXME: hmm, this removes dead particles from the array,
        // the copy operation sucks though...
        if (particle != particleSource)
        {
            *particle = *particleSource;
        }

        // update times
        particle->lifeTime += stepTime;
        float relParticleAge = particle->lifeTime * particle->oneDivMaxLifeTime;
        if ((relParticleAge >= 0.0f) && (relParticleAge < 1.0f))
        {
            // get pointer to anim curves
            int curveIndex = int(relParticleAge * float(ParticleTimeDetail));
            curveIndex = n_iclamp(curveIndex, 0, ParticleTimeDetail - 1);
            float* curCurves = &this->pStaticCurves[curveIndex * CurveTypeCount];

            // compute acceleration vector
            acc = windVector * curCurves[ParticleAirResistance];
            acc.y += this->gravity;
            acc *= curCurves[ParticleMass];

            // update particle
            particle->acc = acc;
            particle->position += particle->velocity * (stepTime * curCurves[ParticleVelocityFactor]);
            particle->velocity += acc * stepTime;
            particle->rotation += curCurves[ParticleRotationVelocity] * particle->rotationVariation * stepTime;

            // update boundary values
            this->box.extend(particle->position);

            // advance to next particle
            particle++;
        }
        else
        {
            // particle's lifetime is over
            // don't advance particle pointer
            this->particleCount--;
            particleIndex--;
        }
        particleSource++;
    }
}

//------------------------------------------------------------------------------
/**
    Checks if the particle system should go to sleep because it is
    too far away or was invisible for some time.
*/
bool
nParticle2Emitter::CheckInvisible(float deltaTime)
{
    // check for activity distance
    const matrix44& viewer = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView);
    vector3 emitterViewer = viewer.pos_component() - this->transform.pos_component();

    if ((emitterViewer.len() >= this->activityDistance) ||                 // viewer out of range
        (!this->frameWasRendered && this->looping && this->hasLooped))     // skip if invisible, looping and has looped at least once
    {
        // adjust starttime by missed timedelta
        this->startTime += deltaTime;
        this->lastEmission += deltaTime;
        if (!this->frameWasRendered)
        {
            this->invisibleTime += deltaTime;
        }
        else
        {
            this->invisibleTime = 0.0f;
        }

        // go to sleep after beeing invisible for too long
        if (this->looping && this->hasLooped && !this->isSleeping && (this->invisibleTime > 3.0f))
        {
            this->isSleeping = true;
            this->DeleteParticles();
        }
        return true;
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Updates the particle system. This will create new particles.

    -04-Dec-06  kims  Changed that particles to be emitted on a surface.
*/
void
nParticle2Emitter::Update(float curTime)
{
    if (!this->IsSetup())
    {
        // not completely setup yet
        return;
    }

    // need to initialize?
    bool firstRun = false;
    if (!this->IsValid())
    {
        this->Initialize();
        firstRun = true;
    }

    // check for time exception
    int numSteps = 1;
    float stepSize = 0.0f;
    if (firstRun || (curTime < this->lastEmission))
    {
        // a time exception has occured, or we are run for the first
        // time, so reset the particle system
        if (this->precalcTime > 0.0f)
        {
            stepSize = 0.1f;    // calculate 1/10s steps
            numSteps = int(this->precalcTime / stepSize) + 1;
            curTime -= (numSteps - 1) * stepSize;
        }
        this->particleCount = 0;
        this->startTime = curTime;
        this->lastEmission = curTime;
        this->remainingTime = 0;
    }

    // for each step...
    int curStep;
    for (curStep = 0; curStep < numSteps; curStep++)
    {
        // calculate timestep
        float deltaTime = float(curTime - this->lastEmission);
        n_assert(deltaTime >= 0.0f);

        // compute stepTime, which is deltaTime manipulated by
        // time manipulator curve
        float relAge = float((curTime - this->startTime - this->startDelay) / this->emissionDuration);
        int curveIndex = int(relAge * ParticleTimeDetail);
        curveIndex = n_iclamp(curveIndex, 0, ParticleTimeDetail - 1);
        float stepTime = n_abs(deltaTime * this->pStaticCurves[curveIndex * CurveTypeCount + TimeManipulator]);
        n_assert(stepTime >= 0.0f);

        // invisibility- and out-of-range-check
        // (only if this is not an initial update)
        if ((1 == numSteps) && this->CheckInvisible(deltaTime))
        {
            return;
        }

        this->frameWasRendered = false;
        this->invisibleTime = 0;

        // update existing particle positions, remove dead ones
        this->CalculateStep(stepTime);

        // emit new particles if we are inside the emissiontimeframe
        if ((curTime >= this->startTime) && (lastEmission < this->startTime + this->startDelay + this->emissionDuration))
        {
            if (curTime >= (this->startTime + this->startDelay))
            {
                // FIXME: it may happen that relAge becomes greater 1.0 here!
                float relAge = float((curTime - this->startTime - this->startDelay) / this->emissionDuration);
                int curveIndex = int(relAge * ParticleTimeDetail);
                curveIndex = n_iclamp(curveIndex, 0, ParticleTimeDetail - 1);
                float* curCurves = &this->pStaticCurves[curveIndex * CurveTypeCount];
                float* vertices = this->refEmitterMesh->LockVertices();
                int vertexWidth = this->refEmitterMesh->GetVertexWidth();
                ushort* indices = this->refEmitterMesh->LockIndices();
                const nMeshGroup& meshGroup = this->refEmitterMesh->Group(this->emitterMeshGroupIndex);
                int firstIndex  = meshGroup.FirstIndex;
                int numIndices = meshGroup.NumIndices;
                matrix33 transform33(this->transform.x_component(), this->transform.y_component(), this->transform.z_component());
                vector3 emissionPos, emissionNormal;

                float curEmissionFrequency = curCurves[EmissionFrequency];

                // for correct emission we perform 1s/freq - steps
                float timeToDo = stepTime + this->remainingTime;
                if (0.0f < curEmissionFrequency)
                {
                    float emitTimeStep = 1.0f / curEmissionFrequency;

                    while (timeToDo >= emitTimeStep)
                    {
                        float particleEmissionLifeTime = timeToDo;
                        float oneDivLifeTime = 1.0f;
                        if (0 != curCurves[ParticleLifeTime])
                        {
                            oneDivLifeTime = 1.0f / curCurves[ParticleLifeTime];
                        }

                        if (this->particleCount < this->maxParticleCount)
                        {
                            // emit a new particle
                            nParticle2* newParticle = &this->particles[this->particleCount];
                            this->particleCount++;

                            if (this->emitOnSurface)
                            {
                                int faceIndex = int(n_rand(0.0f, numIndices / 3 - TINY));
                                n_assert(faceIndex * 3 + 2 < numIndices);

                                // combine 2 triangles into a parallelogram
                                // get 2 randomized lerp value
                                float lerp0 = n_rand();
                                float lerp1 = n_rand();
                                if (lerp1 + lerp0 > 1)
                                {
                                    // the position is in the other side of the parallelogram
                                    // just turn it back
                                    lerp0 = 1 - lerp0;
                                    lerp1 = 1 - lerp1;
                                }
                                vector3 tmp;

                                // get emission position
                                int indexIndex = firstIndex + faceIndex * 3;
                                const vector3 &vertex0 = *(vector3*)&(vertices[indices[indexIndex + 0] * vertexWidth]);
                                const vector3 &vertex1 = *(vector3*)&(vertices[indices[indexIndex + 1] * vertexWidth]);
                                const vector3 &vertex2 = *(vector3*)&(vertices[indices[indexIndex + 2] * vertexWidth]);
                                tmp.lerp(vertex0, vertex1, lerp0);
                                tmp += (vertex2 - vertex0) * lerp1;
                                emissionPos.set(tmp);
                                emissionPos = this->transform * emissionPos;

                                // get emission normal
                                const vector3 &normal0 = *(vector3*)&(vertices[indices[indexIndex + 0] * vertexWidth + 3]);
                                const vector3 &normal1 = *(vector3*)&(vertices[indices[indexIndex + 1] * vertexWidth + 3]);
                                const vector3 &normal2 = *(vector3*)&(vertices[indices[indexIndex + 2] * vertexWidth + 3]);
                                tmp.lerp(normal0, normal1, lerp0);
                                tmp += (normal2 - normal0) * lerp1;
                                emissionNormal.set(tmp);
                                emissionNormal = transform33 * emissionNormal;
                            }
                            else
                            {
                                // get emission position
                                int indexIndex = firstIndex + int(n_rand(0.0f, float(numIndices - 1)));
                                float* vertexPtr = &(vertices[indices[indexIndex] * vertexWidth]);
                                emissionPos.set(vertexPtr[0], vertexPtr[1], vertexPtr[2]);
                                emissionPos = this->transform * emissionPos;

                                // get emission normal
                                emissionNormal.set(vertexPtr[3], vertexPtr[4], vertexPtr[5]);
                                emissionNormal = transform33 * emissionNormal;
                            }

                            // find orthogonal vectors to spread normal vector
                            vector3 ortho1;
                            ortho1 = emissionNormal.findortho();
                            ortho1.norm();
                            vector3 normBackup = emissionNormal;
                            float spreadMin = curCurves[ParticleSpreadMin];
                            float spreadMax = curCurves[ParticleSpreadMax];
                            spreadMin = n_min(spreadMin, spreadMax);
                            float spread = n_lerp(spreadMin, spreadMax, n_rand());
                            float rotRandom = n_rand() * 360.0f;
                            emissionNormal.rotate(ortho1, n_deg2rad(spread));
                            emissionNormal.rotate(normBackup, n_deg2rad(rotRandom));

                            float velocityVariation = 1.0f - n_rand(0.0f, this->particleVelocityRandomize);
                            float startVelocity = curCurves[ParticleStartVelocity] * velocityVariation;

                            // apply texture tiling
                            // uvmax and uvmin are arranged a bit strange, because they need to be flipped
                            // horizontally and be rotated
                            float vStep = 1.0f / float(this->tileTexture);
                            int tileNr = int(n_rand(0.0f, float(this->tileTexture)));

                            newParticle->uvmin.set(1.0f, vStep * float(tileNr));
                            newParticle->uvmax.set(0.0f, newParticle->uvmin.y + vStep);
                            newParticle->lifeTime = particleEmissionLifeTime;
                            newParticle->oneDivMaxLifeTime = oneDivLifeTime;
                            newParticle->position = emissionPos;
                            newParticle->rotation = n_lerp(this->startRotationMin, this->startRotationMax, n_rand());
                            newParticle->rotationVariation = 1.0f - n_rand() * this->particleRotationRandomize;
                            if (this->randomRotDir && (n_rand() < 0.5f))
                            {
                                newParticle->rotationVariation = -newParticle->rotationVariation;
                            }
                            newParticle->velocity = emissionNormal * startVelocity;
                            newParticle->startPos = newParticle->position;
                            newParticle->sizeVariation = 1.0f - n_rand() * this->particleSizeRandomize;

                            // add velocity*lifetime
                            // FIXME FLOH: why this??
                            newParticle->position += newParticle->velocity * newParticle->lifeTime;
                        }
                        timeToDo -= emitTimeStep;
                    }
                }
                this->remainingTime = timeToDo;
                this->refEmitterMesh->UnlockVertices();
                this->refEmitterMesh->UnlockIndices();
            }
        }
        else
        {
            if (this->looping)
            {
                this->startTime = curTime;
                this->remainingTime = 0;
                this->hasLooped = true;
            }
        }
        this->lastEmission = curTime;
        curTime += stepSize;
    }
}

//------------------------------------------------------------------------------
/**
    Render as pure quad

    FIXME: CLEANUP
*/
int nParticle2Emitter::RenderPure(float* dstVertices,int maxVertices)
{
    int curIndex  = 0;
    int curVertex = 0;
    tParticleVertex myVertex;

    nParticle2* particle = this->particles;
    int particlePitch = 1;
    if (this->renderOldestFirst)
    {
        // reverse iterating order
        particlePitch = -1;
        particle = &this->particles[particleCount -1];
    };

    tParticleVertex*    destPtr = (tParticleVertex*)dstVertices;

    int curveIndex;
    float* curCurves;

    const matrix44& viewer = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView);
    myVertex.vel = viewer.x_component();

    int p;
    for (p = 0; p < particleCount ; p++)
    {
        // life-time-check is not needed, it is assured that the relative age is >=0 and <1
        curveIndex = (int)((particle->lifeTime * particle->oneDivMaxLifeTime) * (float)ParticleTimeDetail);
        curveIndex = n_iclamp(curveIndex, 0, ParticleTimeDetail - 1);
        curCurves = &this->pStaticCurves[curveIndex*CurveTypeCount];

        myVertex.pos = particle->position;
        myVertex.scale = curCurves[ParticleScale]*particle->sizeVariation;
        myVertex.color = curCurves[StaticRGBCurve];

        myVertex.u = particle->uvmax.x;
        myVertex.v = particle->uvmin.y;
        myVertex.rotation = particle->rotation+PI/4.0f + PI/2.0f;
        myVertex.alpha = curCurves[ParticleAlpha];
        destPtr[0] = myVertex;
        destPtr[3] = myVertex;

        myVertex.u = particle->uvmax.x;
        myVertex.v = particle->uvmax.y;
        myVertex.rotation += PI/2.0;
        destPtr[1] = myVertex;

        myVertex.u = particle->uvmin.x;
        myVertex.v = particle->uvmax.y;
        myVertex.rotation += PI/2.0;
        destPtr[2] = myVertex;
        destPtr[4] = myVertex;

        myVertex.u = particle->uvmin.x;
        myVertex.v = particle->uvmin.y;
        myVertex.rotation += PI/2.0;
        destPtr[5] = myVertex;

        destPtr += 6;


        curVertex += 6;
        if (curVertex > maxVertices-6)
        {
            this->particleMesh.Swap(curVertex, dstVertices);
            destPtr = (tParticleVertex*)dstVertices;
            curVertex = 0;
        }

        particle += particlePitch;
    }

    return curVertex;
};

//------------------------------------------------------------------------------
/**
    Render stretched

    FIXME: CLEANUP
*/
int nParticle2Emitter::RenderStretched(float* dstVertices,int maxVertices)
{
    int curVertex = 0;
    tParticleVertex myVertex;

    int particlePitch = 1;
    int particleOffset = 0;
    if (this->renderOldestFirst)
    {
        // reverse iterating order
        particlePitch = -1;
        particleOffset = particleCount -1;
    };

    float viewFadeOut = 0.0f;
    if (this->viewAngleFade) viewFadeOut = 256.0f;

    tParticleVertex*    destPtr = (tParticleVertex*)dstVertices;

    // ok, let's stretch
    vector3 stretchPos;
    int p;
    for (p = 0; p < particleCount ; p++)
    {
        nParticle2* particle = &particles[particleOffset];
        float relParticleAge = particle->lifeTime * particle->oneDivMaxLifeTime;
        int curveIndex = (int)(relParticleAge * (float)ParticleTimeDetail);
        curveIndex = n_iclamp(curveIndex, 0, ParticleTimeDetail - 1);
        float* curCurves = &this->pStaticCurves[curveIndex*CurveTypeCount];

        // life-time-check is not needed, it is assured that the relative age is >=0 and <1

        float stretchTime = this->particleStretch;
        if (stretchTime>particle->lifeTime) stretchTime = particle->lifeTime;
        stretchPos = particle->position - (particle->velocity-particle->acc*(stretchTime*0.5f)) * (stretchTime*curCurves[ParticleVelocityFactor]);
        if (this->stretchToStart) stretchPos = particle->startPos;

        float alpha = curCurves[ParticleAlpha] + viewFadeOut;

        myVertex.pos = particle->position;
        myVertex.scale = curCurves[ParticleScale]*particle->sizeVariation;
        myVertex.color = curCurves[StaticRGBCurve];
        myVertex.vel = particle->position - stretchPos;

        myVertex.u = particle->uvmax.x;
        myVertex.v = particle->uvmin.y;
        myVertex.rotation = PI/4.0;
        myVertex.alpha = alpha;
        destPtr[0] = myVertex;
        destPtr[3] = myVertex;

        myVertex.u = particle->uvmax.x;
        myVertex.v = particle->uvmax.y;
        myVertex.rotation += PI/2.0;
        myVertex.pos = stretchPos;
        destPtr[1] = myVertex;


        myVertex.u = particle->uvmin.x;
        myVertex.v = particle->uvmax.y;
        myVertex.rotation += PI/2.0;
        destPtr[2] = myVertex;
        destPtr[4] = myVertex;

        myVertex.u = particle->uvmin.x;
        myVertex.v = particle->uvmin.y;
        myVertex.rotation += PI/2.0f;
        myVertex.pos = particle->position;
        destPtr[5] = myVertex;

        curVertex += 6;
        destPtr += 6;

        if (curVertex > maxVertices-6)
        {
            this->particleMesh.Swap(curVertex, dstVertices);
            curVertex = 0;
            destPtr = (tParticleVertex*)dstVertices;
        }

        particleOffset += particlePitch;
    };
    return curVertex;
};

//------------------------------------------------------------------------------
/**
    Render stretched and smooth

    FIXME: CLEANUP
*/
int nParticle2Emitter::RenderStretchedSmooth(float* dstVertices,int maxVertices)
{
    int curVertex = 0;
    tParticleVertex myVertex;

    int particlePitch = 1;
    int particleOffset = 0;
    if (this->renderOldestFirst)
    {
        // reverse iterating order
        particlePitch = -1;
        particleOffset = particleCount -1;
    };

    float oneDivStretchDetail = 1.0f / (float)this->stretchDetail;

    // ok, let's stretch
    vector3 stretchPos;
    vector3 velPitch;
    vector3 velPitchHalf;

    // set coded flag for viewangle fading
    float viewFadeOut = 0.0f;
    if (this->viewAngleFade) viewFadeOut = 256.0f;

    tParticleVertex*    destPtr = (tParticleVertex*)dstVertices;

    int p;
    for (p = 0; p < this->particleCount ; p++)
    {
        nParticle2* particle = &particles[particleOffset];
        float relParticleAge = particle->lifeTime * particle->oneDivMaxLifeTime;
        int curveIndex = (int)(relParticleAge * (float)ParticleTimeDetail);
        curveIndex = n_iclamp(curveIndex, 0, ParticleTimeDetail - 1);
        float* curCurves = &this->pStaticCurves[curveIndex*CurveTypeCount];

        // calculate stretch steps
        float stretchTime = this->particleStretch;
        if (stretchTime>particle->lifeTime) stretchTime = particle->lifeTime;
        float stretchStep = -(stretchTime * oneDivStretchDetail);
        velPitch = particle->acc * stretchStep;
        velPitchHalf = velPitch * 0.5f;
        float stretchStepVel = stretchStep * curCurves[ParticleVelocityFactor];

        float scale = curCurves[ParticleScale]*particle->sizeVariation;
        float vPitch = (particle->uvmax.y - particle->uvmin.y) * oneDivStretchDetail;

        myVertex.color = curCurves[StaticRGBCurve];
        myVertex.v = particle->uvmin.y;
        myVertex.pos = particle->position;
        myVertex.vel = particle->velocity;

        myVertex.alpha = curCurves[ParticleAlpha] + viewFadeOut;
        myVertex.scale = scale;

        int d;
        for (d = 0; d < this->stretchDetail; d++)
        {
            // life-time-check is not needed, it is assured that the relative age is >=0 and <1
            myVertex.u = particle->uvmin.x;
            myVertex.rotation = 3.0f*PI/2.0f;
            destPtr[0] = myVertex;
            destPtr[3] = myVertex;

            myVertex.rotation = PI/2.0f;
            myVertex.u = particle->uvmax.x;
            destPtr[5] = myVertex;

            myVertex.rotation = 3.0f*PI/2.0f;
            myVertex.u = particle->uvmin.x;
            myVertex.v += vPitch;
            myVertex.pos += (myVertex.vel + velPitchHalf) * stretchStepVel;
            myVertex.vel += velPitch;
            destPtr[1] = myVertex;

            myVertex.rotation = PI/2.0f;
            myVertex.u = particle->uvmax.x;
            destPtr[2] = myVertex;
            destPtr[4] = myVertex;

            destPtr += 6;
            curVertex += 6;
            if (curVertex > maxVertices-6)
            {
                this->particleMesh.Swap(curVertex, dstVertices);
                destPtr = (tParticleVertex*)dstVertices;
                curVertex = 0;
            }
        };

        particleOffset += particlePitch;
    };
    return curVertex;
};


//------------------------------------------------------------------------------
/**
    Render the Particles

    FIXME: CLEANUP
*/
void nParticle2Emitter::Render(float curTime)
{
    if ((!this->IsSetup())||(!this->IsValid())) return;

    if (this->isSleeping)    // do we have to wakeup ?
    {
        this->isSleeping = false;
        // reallocate particles
        this->AllocateParticles();
        n_assert(0 != this->particles);

        this->frameWasRendered = true;
        this->Update(curTime - 0.001f);    // trigger with a little difference, so that the emitter will reset

        // ok, we're up-to-date again
    }

    if (!this->particleMesh.IsValid())
    {
        this->particleMesh.Initialize(nGfxServer2::TriangleList,
            nMesh2::Coord | nMesh2::Normal | nMesh2::Uv0 | nMesh2::Color ,
            nMesh2::WriteOnly | nMesh2::NeedsVertexShader,
            false, "particle2_", true);
        n_assert(this->particleMesh.IsValid());
    }

    float* dstVertices = 0;
    int maxVertices = 0;
    int remVertices = 0;
    this->particleMesh.Begin(dstVertices, maxVertices);

    if ((this->particleStretch == 0.0f)&&(!this->stretchToStart))
        remVertices = RenderPure(dstVertices,maxVertices);
    else
    {
        if (this->stretchToStart || (this->stretchDetail == 1))
        {
            remVertices = RenderStretched(dstVertices,maxVertices);
        } else {
            remVertices = RenderStretchedSmooth(dstVertices,maxVertices);
        }
    }

    // Draw
    this->particleMesh.End(remVertices);
    this->frameWasRendered = true;
}

//------------------------------------------------------------------------------
/**
    FIXME: CLEANUP
*/
void
nParticle2Emitter::Initialize()
{
    n_assert(0 != this->pStaticCurves);

    // calculate maximum number of particles
    float maxFreq = 0 , maxLife = 0;
    int i;
    for (i = 0; i < ParticleTimeDetail ; i++)
    {
        if (this->pStaticCurves[i*CurveTypeCount+EmissionFrequency] > maxFreq)
            maxFreq = this->pStaticCurves[i*CurveTypeCount+EmissionFrequency];
        if (this->pStaticCurves[i*CurveTypeCount+ParticleLifeTime] > maxLife)
            maxLife = this->pStaticCurves[i*CurveTypeCount+ParticleLifeTime];
    }
    this->maxParticleCount = (int)n_ceil(maxFreq * maxLife);
    this->maxParticleCount = n_max(1, this->maxParticleCount);
    // allocate array
    this->AllocateParticles();
    n_assert(0 != this->particles);
    // reset particles
    this->particleCount = 0;


    this->isValid = true;
}

//------------------------------------------------------------------------------
/**
    FIXME: CLEANUP
*/
void
nParticle2Emitter::NotifyCurvesChanged()
{
    n_assert(0 != this->pStaticCurves);

    if (this->particles != 0)
    {
        // we need to rearrange the particlearray, because the curves have changed

        float maxFreq = 0, maxLife = 0;
        int i;
        for (i = 0; i < ParticleTimeDetail; i++)
        {
            if (this->pStaticCurves[i * CurveTypeCount + EmissionFrequency] > maxFreq)
                maxFreq = this->pStaticCurves[i * CurveTypeCount + EmissionFrequency];
            if (this->pStaticCurves[i * CurveTypeCount + ParticleLifeTime] > maxLife)
                maxLife = this->pStaticCurves[i * CurveTypeCount + ParticleLifeTime];
        }
        int newMaxParticleCount = (int)n_ceil(maxFreq * maxLife);
        newMaxParticleCount = n_max(1, newMaxParticleCount);
        if (newMaxParticleCount > this->maxParticleCount || newMaxParticleCount < this->maxParticleCount / 2)
        {
            // allocate array
            nParticle2* newPtr = n_new_array(nParticle2, newMaxParticleCount);
            n_assert(0 != newPtr);

            int partsToCopy = this->particleCount;
            if (partsToCopy > newMaxParticleCount)
                partsToCopy = newMaxParticleCount;

            memcpy(newPtr, this->particles, partsToCopy * sizeof(nParticle2));

            // delete old array
            n_delete_array(this->particles);

            // set new values
            this->particleCount = partsToCopy;
            this->particles = newPtr;
            this->maxParticleCount = newMaxParticleCount;
        }
    }
}
