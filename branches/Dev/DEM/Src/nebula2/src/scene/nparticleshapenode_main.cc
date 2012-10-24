//------------------------------------------------------------------------------
//  nparticleshapenode_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nparticleshapenode.h"
#include "variable/nvariableserver.h"
#include "scene/nrendercontext.h"
#include "scene/nsceneserver.h"
#include "scene/nanimator.h"

nNebulaClass(nParticleShapeNode, "nshapenode");

//------------------------------------------------------------------------------
/**
*/
nParticleShapeNode::nParticleShapeNode() :
    emissionDuration(10.0),
    loop(true),
    activityDistance(10.0f),
    spreadAngle(0.0f),
    birthDelay(0.0f),
    emitterVarIndex(-1),
    renderOldestFirst(true)
{
    int i;
    for (i=0; i<4; i++)
    {
        this->curves[nParticleEmitter::ParticleVelocityFactor].keyFrameValues[i] = 1.0;
    }

    // obtain variable handles
    this->timeHandle = nVariableServer::Instance()->GetVariableHandleByName("time");
    this->windHandle = nVariableServer::Instance()->GetVariableHandleByName("wind");
}

//------------------------------------------------------------------------------
/**
*/
nParticleShapeNode::~nParticleShapeNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Compute the resulting modelview matrix and set it in the scene
    server as current modelview matrix.

    *** FIXME FIXME FIXME ***
    * why is emitter setup in RenderTransform() and not in RenderGeometry()???
    * generally do some cleanup on the particle subsystem

    - 28-Jan-04     daniel  emitter setup moved here from RenderTransform()
*/
bool
nParticleShapeNode::RenderTransform(nSceneServer* sceneServer,
                                    nRenderContext* renderContext,
                                    const matrix44& parentMatrix)
{
    n_assert(sceneServer);
    n_assert(renderContext);
    //this->InvokeAnimators(nAnimator::Transform, renderContext);

    // get emitter from render context
    nVariable& varEmitter = renderContext->GetLocalVar(this->emitterVarIndex);
    int emitterKey = varEmitter.GetInt();
    nParticleEmitter* emitter = nParticleServer::Instance()->GetParticleEmitter(emitterKey);

    // keep emitter alive
    if (0 == emitter)
    {
        // need new emitter
        emitter = nParticleServer::Instance()->NewParticleEmitter();
        n_assert(0 != emitter);

        varEmitter.SetInt(emitter->GetKey());
    }

    // setup emitter
    emitter->SetMeshGroupIndex(this->groupIndex);
    emitter->SetEmitterMesh(this->refMesh.get());
    emitter->SetTransform(this->tform.getmatrix() * parentMatrix);
    nVariable* windVar = renderContext->GetVariable(this->windHandle);
    n_assert2(windVar, "No 'wind' variable provided by application!");
    emitter->SetWind(windVar->GetFloat4());

    // set emitter settings
    emitter->SetEmissionDuration(float(this->emissionDuration));
    emitter->SetLoop(this->loop);
    emitter->SetActivityDistance(this->activityDistance);
    emitter->SetSpreadAngle(this->spreadAngle);
    emitter->SetBirthDelay(this->birthDelay);
    emitter->SetStartRotation(this->startRotation);
    emitter->SetRenderOldestFirst(this->renderOldestFirst);
    int curveType;
    for (curveType = 0; curveType < nParticleEmitter::CurveTypeCount; curveType++)
    {
        emitter->SetCurve((nParticleEmitter::CurveType) curveType, this->curves[curveType]);
    }
    emitter->SetRGBCurve(this->rgbCurve);

    sceneServer->SetModelTransform(matrix44::identity);

    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nParticleShapeNode::RenderContextCreated(nRenderContext* renderContext)
{
    nShapeNode::RenderContextCreated(renderContext);

    // see if resources need to be reloaded
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }

    nParticleEmitter* emitter = nParticleServer::Instance()->NewParticleEmitter();
    n_assert(0 != emitter);

    // put emitter key in render context
    this->emitterVarIndex = renderContext->AddLocalVar(nVariable(0, emitter->GetKey()));
}

//------------------------------------------------------------------------------
/**
    - 15-Jan-04     floh    AreResourcesValid()/LoadResource() moved to scene server
    - 28-Jan-04     daniel  emitter setup moved to RenderTransform()
*/
void
nParticleShapeNode::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    nShapeNode::Attach(sceneServer, renderContext);
}

//------------------------------------------------------------------------------
/**
    This method must return the mesh usage flag combination required by
    this shape node class. Subclasses should override this method
    based on their requirements.

    @return     a combination of nMesh2::Usage flags
*/
int
nParticleShapeNode::GetMeshUsage() const
{
    return nMesh2::ReadOnly | nMesh2::PointSprite | nMesh2::NeedsVertexShader;
}

//------------------------------------------------------------------------------
/**
    Perform pre-instance-rendering of particle system.
    FIXME: check if this is the optimal setup for the new instance
    rendering!
*/
bool
nParticleShapeNode::ApplyGeometry(nSceneServer* /*sceneServer*/)
{
    return true;
}

//------------------------------------------------------------------------------
/**
    - 15-Jan-04     floh    AreResourcesValid()/LoadResource() moved to scene server
*/
bool
nParticleShapeNode::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    const nVariable& varEmitter = renderContext->GetLocalVar(this->emitterVarIndex);
    int emitterKey = varEmitter.GetInt();
    nVariable* timeVar = renderContext->GetVariable(this->timeHandle);
    n_assert2(timeVar, "No 'time' variable provided by application!");
    float curTime = timeVar->GetFloat();
    n_assert(curTime >= 0.0f);

    nParticleEmitter* emitter = nParticleServer::Instance()->GetParticleEmitter(emitterKey);
    n_assert(0 != emitter);
    emitter->Trigger(curTime);
    emitter->Render(curTime);

    return true;
}

//------------------------------------------------------------------------------
/**
    Returns the current emitter
*/
nParticleEmitter*
nParticleShapeNode::GetEmitter(nRenderContext* renderContext)
{
    // get emitter from render context
    nVariable& varEmitter = renderContext->GetLocalVar(this->emitterVarIndex);
    int emitterKey = varEmitter.GetInt();
    return nParticleServer::Instance()->GetParticleEmitter(emitterKey);
}

