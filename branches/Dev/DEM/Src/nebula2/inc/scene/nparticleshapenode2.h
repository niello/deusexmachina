#ifndef N_PARTICLESHAPENODE2_H
#define N_PARTICLESHAPENODE2_H
//------------------------------------------------------------------------------
/**
    @class nParticleShapeNode2
    @ingroup Scene

    A shape node representing a particle system.

    -04-Dec-06  kims  Changed that particles can be emitted on a surface.

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nshapenode.h"
#include "gfx2/ndynamicmesh.h"
#include "particle/nparticle2emitter.h"
#include "particle/nparticleserver2.h"
#include "mathlib/envelopecurve.h"
#include "mathlib/vector3envelopecurve.h"
#include "variable/nvariable.h"

class nRenderContext;
//------------------------------------------------------------------------------
class nParticleShapeNode2 : public nShapeNode
{
public:
    /// constructor
    nParticleShapeNode2();
    /// destructor
    virtual ~nParticleShapeNode2();
    /// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// called by app when new render context has been created for this object
    virtual void RenderContextCreated(nRenderContext* renderContext);
    /// called by app when render context has been destroyed
    virtual void RenderContextDestroyed(nRenderContext* renderContext);
    /// called by nSceneServer when object is attached to scene
    virtual void Attach(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// perform pre-instancing rendering of geometry
    virtual bool ApplyGeometry(nSceneServer* sceneServer);
    /// render geometry
    virtual bool RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext);
    /// get the mesh usage flags required by this shape node
    virtual int GetMeshUsage() const;
    /// update transform and render into scene server
    virtual bool RenderTransform(nSceneServer* sceneServer, nRenderContext* renderContext, const matrix44& parentMatrix);

    /// set if invisible or not
    void SetInvisible(bool value);
    /// set the end time
    void SetEmissionDuration(float time);
    /// get the emission duration
    float GetEmissionDuration() const;
    /// set if loop emitter or not
    void SetLoop(bool b);
    /// is loop emitter ?
    bool GetLoop() const;
    /// set the activity distance
    void SetActivityDistance(float f);
    /// get the activity distance
    float GetActivityDistance() const;
    /// set the maximum start rotation angle
    void SetStartRotationMin(float f);
    /// set the maximum start rotation angle
    void SetStartRotationMax(float f);
    /// set gravity
    void SetGravity(float f);
    /// set amount of stretching
    void SetParticleStretch(float value);
    void SetParticleVelocityRandomize(float value);
    void SetParticleRotationRandomize(float value);
    void SetParticleSizeRandomize(float value);
    void SetRandomRotDir(bool value);
    /// set texture tiling parts
    void SetTileTexture(int value);
    /// set if texture should be stretched to the emission start point
    void SetStretchToStart(bool value);
    /// set precalc time
    void SetPrecalcTime(float value);
    /// set whether to render oldest or youngest particles first
    void SetRenderOldestFirst(bool b);
    /// set whether to render oldest or youngest particles first
    void SetStretchDetail(int value);
    /// set whether to render oldest or youngest particles first
    void SetViewAngleFade(bool value);
    /// get whether to render oldest or youngest particles first
    bool GetRenderOldestFirst() const;
    /// set start delay
    void SetStartDelay(float value);
    /// set emit on surface or on vertex
    void SetEmitOnSurface(bool value);
    /// get emit on surface or on vertex
    bool GetEmitOnSurface() const;

    /// set one of the envelope curves (not the color)
    void SetCurve(nParticle2Emitter::CurveType curveType, const nEnvelopeCurve& curve);
    /// get one of the envelope curves
    const nEnvelopeCurve& GetCurve(nParticle2Emitter::CurveType curveType) const;

    /// set the particle rgb curve
    void SetRGBCurve(const nVector3EnvelopeCurve& curve);
    /// get the particle rgb curve
    const nVector3EnvelopeCurve& GetRGBCurve() const;
    /// Returns the current emitter
    nParticle2Emitter* GetEmitter(nRenderContext* renderContext);

protected:

    int emitterVarIndex;            // index of the emitter in the render context
    float emissionDuration;         // how long shall be emitted ?
    bool loop;                      // loop emitter ?

    float activityDistance;         // distance between viewer and emitter on witch emitter is active
    float startRotationMin;         // minimum angle of rotation at birth
    float startRotationMax;         // maximum angle of rotation at birth
    float gravity;                  // gravity
    float particleStretch;          // amount of stretching
    float particleVelocityRandomize;
    float particleRotationRandomize;
    float particleSizeRandomize;
    float precalcTime;
    bool  randomRotDir;
    int   tileTexture;              // texture tiling
    bool  stretchToStart;           // stretch to point of emission ?
    bool  renderOldestFirst;        // whether to render the oldest particles first or the youngest
    bool  invisible;
    bool  viewAngleFade;
    int   stretchDetail;
    float startDelay;
    bool  emitOnSurface;            // emit particles from surface or from vertex

    nEnvelopeCurve curves[nParticle2Emitter::CurveTypeCount];
    nVector3EnvelopeCurve rgbCurve;

    nVariable::Handle timeHandle;
    nVariable::Handle windHandle;

    bool        curvesValid;
    float staticCurve[nParticle2Emitter::ParticleTimeDetail][nParticle2Emitter::CurveTypeCount];
};

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode2::SetEmissionDuration(float time)
{
    this->emissionDuration = time;
}

//------------------------------------------------------------------------------
/**
*/
inline float nParticleShapeNode2::GetEmissionDuration() const
{
    return this->emissionDuration;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode2::SetLoop(bool b)
{
    this->loop = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool nParticleShapeNode2::GetLoop() const
{
    return this->loop;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode2::SetActivityDistance(float f)
{
    this->activityDistance = f;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode2::SetStartRotationMin(float f)
{
    this->startRotationMin = f;
}
//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode2::SetStartRotationMax(float f)
{
    this->startRotationMax = f;
}
//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode2::SetGravity(float f)
{
    this->gravity = f;
}

//------------------------------------------------------------------------------
/**
*/
inline float nParticleShapeNode2::GetActivityDistance() const
{
    return this->activityDistance;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetRenderOldestFirst(bool b)
{
    this->renderOldestFirst = b;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetParticleStretch(float value)
{
    this->particleStretch = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetPrecalcTime(float value)
{
    this->precalcTime = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetParticleVelocityRandomize(float value)
{
    this->particleVelocityRandomize = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetParticleRotationRandomize(float value)
{
    this->particleRotationRandomize = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetParticleSizeRandomize(float value)
{
    this->particleSizeRandomize = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetRandomRotDir(bool value)
{
    this->randomRotDir = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetTileTexture(int value)
{
    this->tileTexture = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetStretchToStart(bool value)
{
    this->stretchToStart = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticleShapeNode2::GetRenderOldestFirst() const
{
    return this->renderOldestFirst;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetCurve(nParticle2Emitter::CurveType curveType, const nEnvelopeCurve& curve)
{
    n_assert(curveType < nParticle2Emitter::CurveTypeCount);
    n_assert(curveType >= 0);
    this->curvesValid = false;
    this->curves[curveType].SetParameters(curve);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nEnvelopeCurve&
nParticleShapeNode2::GetCurve(nParticle2Emitter::CurveType curveType) const
{
    n_assert(curveType < nParticle2Emitter::CurveTypeCount);
    n_assert(curveType >= 0);
    return this->curves[curveType];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetRGBCurve(const nVector3EnvelopeCurve& curve)
{
    this->curvesValid = false;
    this->rgbCurve.SetParameters(curve);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nVector3EnvelopeCurve&
nParticleShapeNode2::GetRGBCurve() const
{
    return this->rgbCurve;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetInvisible(bool value)
{
    this->invisible = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetStretchDetail(int value)
{
    this->stretchDetail = value;
}
//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetViewAngleFade(bool value)
{
    this->viewAngleFade = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetStartDelay(float value)
{
    this->startDelay = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode2::SetEmitOnSurface(bool value)
{
    this->emitOnSurface = value;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticleShapeNode2::GetEmitOnSurface() const
{
    return this->emitOnSurface;
}

//------------------------------------------------------------------------------
#endif
