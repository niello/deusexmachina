#ifndef N_PARTICLE2EMITTER_H
#define N_PARTICLE2EMITTER_H
//------------------------------------------------------------------------------
/**
    @class nParticle2Emitter
    @ingroup NebulaParticleSystem


    The particle emitter class for the Particle2 system.


    -04-Dec-06  kims  Changed that particles can be emitted on a surface.


    (C) 2003 RadonLabs GmbH
*/
#include "particle/nparticleserver2.h"
#include "particle/nparticle2.h"
#include "gfx2/nmesh2.h"
#include "gfx2/ndynamicmesh.h"
#include "mathlib/bbox.h"

class nParticleServer2;

//------------------------------------------------------------------------------
class nParticle2Emitter
{
public:
    // enumeration of the envelope curves
    enum CurveType
    {
        EmissionFrequency = 0,
        ParticleLifeTime,
        ParticleStartVelocity,
        ParticleRotationVelocity,
        ParticleScale,
        ParticleSpreadMin,
        ParticleSpreadMax,
        ParticleAlpha,
        ParticleAirResistance,
        StaticRGBCurve,
        ParticleVelocityFactor,
        ParticleMass,
        TimeManipulator,

        Filler1,            // filler for getting a CurveTypeCount that's power of two
        Filler2,            // filler for getting a CurveTypeCount that's power of two
        Filler3,            // filler for getting a CurveTypeCount that's power of two

        CurveTypeCount,
    };

    // helper structure for writing into the vertex stream fast
    typedef struct tParticleVertex
    {
        vector3 pos;
        vector3 vel;
        float   u;
        float   v;
        float   alpha;      // alpha and uv-offsets (coded)
        float   rotation;
        float   scale;
        float   color;      // colors get coded into 1 float
    } tParticleVertex;

    static const int ParticleTimeDetail = 100;

    /// constructor
    nParticle2Emitter();
    /// destructor
    virtual ~nParticle2Emitter();

    /// set current emitter transformation
    void SetTransform(const matrix44& transform);
    /// get current emitter transformation
    const matrix44& GetTransform() const;
    /// set the start time
    void SetStartTime(float time);
    /// get the start time
    float GetStartTime() const;
    /// set the end time
    void SetEmissionDuration(float time);
    /// get the emission duration
    float GetEmissionDuration() const;
    /// set if loop emitter or not
    void SetLooping(bool b);
    /// is it a loop emitter ?
    bool GetLooping() const;
    /// set the activity distance
    void SetActivityDistance(float f);
    /// get the distance to the viewer beyond which the emitter stops emitting
    float GetActivityDistance() const;
    /// set wether to render oldest or youngest particles first
    void SetRenderOldestFirst(bool b);
    /// get wether to render oldest or youngest particles first
    bool GetRenderOldestFirst() const;

    /// update particles
    void Update(float curTime);
    /// render particles
    void Render(float curTime);

    /// set the mesh that emittes particles
    void SetEmitterMesh(nMesh2*);
    /// get mesh that emits
    nMesh2* GetEmitterMesh() const;
    /// set emitter mesh group index
    void SetEmitterMeshGroupIndex(int index);
    /// get emitter mesh group index
    int GetEmitterMeshGroupIndex() const;

    /// get current particle count
    int GetParticleCount() const;

    /// set bounding box
    void SetBoundingBox(const bbox3& b);
    /// get bounding box
    const bbox3& GetBoundingBox() const;
    /// set the wind
    void SetWind(const nFloat4& wind);
    /// get the wind
    const nFloat4& GetWind() const;

    /// return true if particle system has been initialized
    bool IsValid() const;
    /// initializing, call once after setting parameters
    void Initialize();

    /// signal that all necessary values have been set and emitting can begin
    void SetIsSetup(bool b);
    /// return if emitter is set up
    bool IsSetup() const;

    /// set pointer to parameter curves (identical for all instances of a particle system)
    void SetStaticCurvePtr(float* ptr);
    /// return true if static curve pointer is valid
    bool IsStaticCurvePtrValid() const;
    /// called when remotecontrol (maya) changes the curves
    void NotifyCurvesChanged();

    /// set gravity force
    void SetGravity(float gravity);
    /// set minimum rotation angle at emission
    void SetStartRotationMin(float value);
    /// set maximum rotation angle at emission
    void SetStartRotationMax(float value);
    /// set amount (time) of stretching
    void SetParticleStretch(float value);
    /// set texture tiling parts
    void SetTileTexture(int value);
    /// set if particles should be stretched to the emission startpoint
    void SetStretchToStart(bool value);
    /// set Velocity Randomize
    void SetParticleVelocityRandomize(float value);
    /// set Rotation Randomize
    void SetParticleRotationRandomize(float value);
    /// set Size Randomize
    void SetParticleSizeRandomize(float value);
    /// set random rotation direction
    void SetRandomRotDir(bool value);
    /// set Precalculation time
    void SetPrecalcTime(float value);
    /// set random rotation direction
    void SetStretchDetail(int value);
    /// set random rotation direction
    void SetViewAngleFade(bool value);
    /// set start delay
    void SetStartDelay(float value);
    /// set emit on surface or on vertex
    void SetEmitOnSurface(bool value);
    /// get emit on surface or on vertex
    bool GetEmitOnSurface() const;

protected:
    float* pStaticCurves;
    nDynamicMesh particleMesh;
    nRef<nMesh2> refEmitterMesh;
    int emitterMeshGroupIndex;
    matrix44 transform;
    bbox3 box;
    nFloat4 wind;
    int lastEmissionVertex;
    float startTime;
    float lastEmission;

    // emitter settings
    float emissionDuration;
    bool looping;
    float activityDistance;
    float particleStretch;
    float precalcTime;
    int tileTexture;
    bool renderOldestFirst;
    bool stretchToStart;
    bool randomRotDir;
    bool hasLooped;
    bool frameWasRendered;
    float invisibleTime;
    bool isSleeping;
    int stretchDetail;
    bool viewAngleFade;
    float startDelay;
    /// flag for particles can be emitted on a surface rather than vertices.
    bool emitOnSurface;

    float gravity;
    float startRotationMin;
    float startRotationMax;
    float particleVelocityRandomize;
    float particleRotationRandomize;
    float particleSizeRandomize;

    nParticle2* particles;
    int particleCount;
    int maxParticleCount;

    float remainingTime;

    bool isValid;
    bool isSetup;

private:
    /// not implemented operator to prevent '=' - assignment
    nParticle2Emitter& operator=(const nParticle2Emitter &);
    /// update particles
    void CalculateStep(float fdTime);
    /// render as "normal" particles
    int RenderPure(float* dstVertices, int maxVertices);
    /// render as stretched particles
    int RenderStretched(float* dstVertices, int maxVertices);
    /// expensive-high-quality stretched rendering
    int RenderStretchedSmooth(float* dstVertices, int maxVertices);
    /// allocate particle pool
    void AllocateParticles();
    /// delete particles pool
    void DeleteParticles();
    /// check whether particle emitter is invisible and should go to sleep
    bool CheckInvisible(float deltaTime);
};

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticle2Emitter::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetTransform(const matrix44& m)
{
    this->transform = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
nParticle2Emitter::GetTransform() const
{
    return this->transform;
}

//------------------------------------------------------------------------------
/**
    -04-Dec-06  kims  Added missing member func. Thank kaikai for the patch.
*/
inline
float
nParticle2Emitter::GetStartTime() const
{
    return this->startTime;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetStartTime(float time)
{
    this->startTime = time;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetEmitterMesh(nMesh2* mesh)
{
    this->refEmitterMesh = mesh;
}

//------------------------------------------------------------------------------
/**
*/
inline
nMesh2*
nParticle2Emitter::GetEmitterMesh() const
{
    return this->refEmitterMesh.get();
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetEmitterMeshGroupIndex(int index)
{
    this->emitterMeshGroupIndex = index;
}

//------------------------------------------------------------------------------
/**
*/
inline
int
nParticle2Emitter::GetEmitterMeshGroupIndex() const
{
    return this->emitterMeshGroupIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetEmissionDuration(float time)
{
    this->emissionDuration = time;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nParticle2Emitter::GetEmissionDuration() const
{
    return this->emissionDuration;
}

//------------------------------------------------------------------------------
/**
    -04-Dec-06  kims  Added missing member func. Thank kaikai for the patch.
*/
inline
int
nParticle2Emitter::GetParticleCount() const
{
    return this->particleCount;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetLooping(bool b)
{
    this->looping = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticle2Emitter::GetLooping() const
{
    return this->looping;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetActivityDistance(float f)
{
    this->activityDistance = f;
}

//------------------------------------------------------------------------------
/**
*/
inline
float
nParticle2Emitter::GetActivityDistance() const
{
    return this->activityDistance;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetRenderOldestFirst(bool b)
{
    this->renderOldestFirst = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticle2Emitter::GetRenderOldestFirst() const
{
    return this->renderOldestFirst;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetBoundingBox(const bbox3& b)
{
    this->box = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
const bbox3&
nParticle2Emitter::GetBoundingBox() const
{
    return this->box;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetWind(const nFloat4& wind)
{
    this->wind = wind;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nFloat4&
nParticle2Emitter::GetWind() const
{
    return this->wind;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetIsSetup(bool b)
{
    this->isSetup = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticle2Emitter::IsSetup() const
{
    return this->isSetup;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetStaticCurvePtr(float* ptr)
{
    this->pStaticCurves = ptr;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticle2Emitter::IsStaticCurvePtrValid() const
{
    return (0 != this->pStaticCurves);
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetGravity(float gravity)
{
    this->gravity = gravity;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetStartRotationMin(float value)
{
    this->startRotationMin = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetStartRotationMax(float value)
{
    this->startRotationMax = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetParticleStretch(float value)
{
    this->particleStretch = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetTileTexture(int value)
{
    if (value < 1) value = 1;
    this->tileTexture = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetStretchToStart(bool value)
{
    this->stretchToStart = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetParticleVelocityRandomize(float value)
{
    this->particleVelocityRandomize = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetParticleRotationRandomize(float value)
{
    this->particleRotationRandomize = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetParticleSizeRandomize(float value)
{
    this->particleSizeRandomize = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetPrecalcTime(float value)
{
    this->precalcTime = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetRandomRotDir(bool value)
{
    this->randomRotDir = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetStretchDetail(int value)
{
    this->stretchDetail = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetViewAngleFade(bool value)
{
    this->viewAngleFade = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticle2Emitter::SetStartDelay(float value)
{
    this->startDelay = value;
}

//------------------------------------------------------------------------------
/**
    -04-Dec-06  kims  It is needed for particles can be emitted on a surface. 
                      Thank kaikai for the patch.
*/
inline
void
nParticle2Emitter::SetEmitOnSurface(bool value)
{
    this->emitOnSurface = value;
}

//------------------------------------------------------------------------------
/**
    -04-Dec-06  kims  It is needed for particles can be emitted on a surface. 
                      Thank kaikai for the patch.
*/
inline
bool
nParticle2Emitter::GetEmitOnSurface() const
{
    return this->emitOnSurface;
}

//------------------------------------------------------------------------------
#endif
