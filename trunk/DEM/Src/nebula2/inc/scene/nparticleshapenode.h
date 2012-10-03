#ifndef N_PARTICLESHAPENODE_H
#define N_PARTICLESHAPENODE_H
//------------------------------------------------------------------------------
/**
    @class nParticleShapeNode
    @ingroup Scene

    @brief A shape node representing a particle system.

    See also @ref N2ScriptInterface_nparticleshapenode

    (C) 2004 RadonLabs GmbH
*/
#include "scene/nshapenode.h"
#include "gfx2/ndynamicmesh.h"
#include "particle/nparticleemitter.h"
#include "particle/nparticleserver.h"
#include "variable/nvariable.h"

class nRenderContext;
//------------------------------------------------------------------------------
class nParticleShapeNode : public nShapeNode
{
public:
    /// constructor
    nParticleShapeNode();
    /// destructor
    virtual ~nParticleShapeNode();
    /// object persistency
    //virtual bool SaveCmds(nPersistServer* ps);
    /// called by app when new render context has been created for this object
    virtual void RenderContextCreated(nRenderContext* renderContext);
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

    /// set the end time
    void SetEmissionDuration(nTime time);
    /// get the emission duration
    nTime GetEmissionDuration() const;
    /// set if loop emitter or not
    void SetLoop(bool b);
    /// is loop emitter ?
    bool GetLoop() const;
    /// set the activity distance
    void SetActivityDistance(float f);
    /// get the activity distance
    float GetActivityDistance() const;
    /// set the inner cone
    void SetSpreadAngle(float f);
    /// get the spread angle
    float GetSpreadAngle() const;
    /// set the maximum birth delay
    void SetBirthDelay(float f);
    /// get the maximum birth delay
    float GetBirthDelay() const;
    /// set the maximum start rotation angle
    void SetStartRotation(float f);
    /// get the maximum start rotation angle
    float GetStartRotation() const;
    /// set whether to render oldest or youngest particles first
    void SetRenderOldestFirst(bool b);
    /// get whether to render oldest or youngest particles first
    bool GetRenderOldestFirst() const;

    /// set one of the envelope curves (not the color)
    void SetCurve(nParticleEmitter::CurveType curveType, const nEnvelopeCurve& curve);
    /// get one of the envelope curves
    const nEnvelopeCurve& GetCurve(nParticleEmitter::CurveType curveType) const;

    /// set the particle rgb curve
    void SetRGBCurve(const nVector3EnvelopeCurve& curve);
    /// get the particle rgb curve
    const nVector3EnvelopeCurve& GetRGBCurve() const;
    /// Returns the current emitter
    nParticleEmitter* GetEmitter(nRenderContext* renderContext);

protected:

    int emitterVarIndex;       ///< index of the emitter in the render context
    nTime emissionDuration;    ///< how long shall be emitted ?
    bool loop;                 ///< loop emitter ?

    float activityDistance;    ///< distance between viewer and emitter on witch emitter is active
    float spreadAngle;         ///< angle of emitted particle cone
    float birthDelay;          ///< maximum delay until particle starts to live
    float startRotation;       ///< maximum angle of rotation at birth
    bool  renderOldestFirst;   ///< whether to render the oldest particles first or the youngest

    nEnvelopeCurve curves[nParticleEmitter::CurveTypeCount];
    nVector3EnvelopeCurve rgbCurve;

    nVariable::Handle timeHandle;
    nVariable::Handle windHandle;
};

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode::SetEmissionDuration(nTime time)
{
    this->emissionDuration = time;
}

//------------------------------------------------------------------------------
/**
*/
inline nTime nParticleShapeNode::GetEmissionDuration() const
{
    return this->emissionDuration;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode::SetLoop(bool b)
{
    this->loop = b;
}

//------------------------------------------------------------------------------
/**
*/
inline bool nParticleShapeNode::GetLoop() const
{
    return this->loop;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode::SetActivityDistance(float f)
{
    this->activityDistance = f;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode::SetSpreadAngle(float f)
{
    this->spreadAngle = f;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode::SetBirthDelay(float f)
{
    this->birthDelay = f;
}

//------------------------------------------------------------------------------
/**
*/
inline void nParticleShapeNode::SetStartRotation(float f)
{
    this->startRotation = f;
}

//------------------------------------------------------------------------------
/**
*/
inline float nParticleShapeNode::GetActivityDistance() const
{
    return this->activityDistance;
}

//------------------------------------------------------------------------------
/**
*/
inline float nParticleShapeNode::GetSpreadAngle() const
{
    return this->spreadAngle;
}

//------------------------------------------------------------------------------
/**
*/
inline float nParticleShapeNode::GetBirthDelay() const
{
    return this->birthDelay;
}

//------------------------------------------------------------------------------
/**
*/
inline float nParticleShapeNode::GetStartRotation() const
{
    return this->startRotation;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode::SetRenderOldestFirst(bool b)
{
    this->renderOldestFirst = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nParticleShapeNode::GetRenderOldestFirst() const
{
    return this->renderOldestFirst;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode::SetCurve(nParticleEmitter::CurveType curveType, const nEnvelopeCurve& curve)
{
    n_assert(curveType < nParticleEmitter::CurveTypeCount);
    n_assert(curveType >= 0);
    this->curves[curveType].SetParameters(curve);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nEnvelopeCurve&
nParticleShapeNode::GetCurve(nParticleEmitter::CurveType curveType) const
{
    n_assert(curveType < nParticleEmitter::CurveTypeCount);
    n_assert(curveType >= 0);
    return this->curves[curveType];
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nParticleShapeNode::SetRGBCurve(const nVector3EnvelopeCurve& curve)
{
    this->rgbCurve.SetParameters(curve);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nVector3EnvelopeCurve&
nParticleShapeNode::GetRGBCurve() const
{
    return this->rgbCurve;
}

//------------------------------------------------------------------------------
#endif
