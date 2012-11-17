//------------------------------------------------------------------------------
//  nparticleshapenode2_main.cc
//  (C) 2003 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nparticleshapenode2.h"
#include "variable/nvariableserver.h"
#include "scene/nrendercontext.h"
#include "scene/nsceneserver.h"

nNebulaClass(nParticleShapeNode2, "nshapenode");

//------------------------------------------------------------------------------
/**
*/
nParticleShapeNode2::nParticleShapeNode2() :
    emissionDuration(10.0),
    loop(true),
    activityDistance(10.0f),
    startRotationMin(0.0f),
    startRotationMax(0.0f),
    gravity(-9.81f),
    emitterVarIndex(-1),
    renderOldestFirst(true),
    curvesValid(false),
    invisible(false),
    startDelay(0),
    emitOnSurface(false)
{
    // obtain variable handles
    this->timeHandle = nVariableServer::Instance()->GetVariableHandleByName("time");
    this->windHandle = nVariableServer::Instance()->GetVariableHandleByName("wind");
}

//------------------------------------------------------------------------------
/**
*/
nParticleShapeNode2::~nParticleShapeNode2()
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Compute the resulting modelview matrix and set it in the scene
    server as current modelview matrix.

    -04-Dec-06  kims  Changed that particles can be emitted on a surface.
*/
bool
nParticleShapeNode2::RenderTransform(nSceneServer* sceneServer,
                                    nRenderContext* renderContext,
                                    const matrix44& parentMatrix)
{
    n_assert(sceneServer);
    n_assert(renderContext);
	// Animate here

    // get emitter from render context
    nVariable& varEmitter = renderContext->GetLocalVar(this->emitterVarIndex);
    nParticle2Emitter* emitter = (nParticle2Emitter*)varEmitter.GetObj();

    // sample curves to speed up calculating
    if (!this->curvesValid)
    {
        int c,s;
        for (c = 0; c < nParticle2Emitter::CurveTypeCount ; c++)
            for (s = 0; s < nParticle2Emitter::ParticleTimeDetail; s++)
                this->staticCurve[s][c] = this->curves[c].GetValue((float)s / (float)nParticle2Emitter::ParticleTimeDetail);
        // encode colors
        for (s = 0; s < nParticle2Emitter::ParticleTimeDetail ; s++)
        {
            vector3 col = rgbCurve.GetValue((float)s / (float)nParticle2Emitter::ParticleTimeDetail);
            this->staticCurve[s][nParticle2Emitter::StaticRGBCurve] = (float)((((uint)(col.x*255.0f)) << 16) |
                                                    (((uint)(col.y*255.0f)) << 8) |
                                                    (((uint)(col.z*255.0f))));
        };

        // encode alpha values from [0,1] to [0,255]
        for (s = 0; s < nParticle2Emitter::ParticleTimeDetail ; s++)
            this->staticCurve[s][nParticle2Emitter::ParticleAlpha] = (float)(((int)(this->staticCurve[s][nParticle2Emitter::ParticleAlpha] * 255.0f)));

        this->curvesValid = true;
        if (!emitter->IsStaticCurvePtrValid())
        {
            emitter->SetStaticCurvePtr((float*)&this->staticCurve);
        }
        emitter->NotifyCurvesChanged();
    }
    else if (!emitter->IsStaticCurvePtrValid())
    {
        emitter->SetStaticCurvePtr((float*)&this->staticCurve);
        emitter->NotifyCurvesChanged();
    }

    nVariable* windVar = renderContext->GetVariable(this->windHandle);
    n_assert2(windVar, "No 'wind' variable provided by application!");
    emitter->SetTransform(this->tform.getmatrix() * parentMatrix);
    emitter->SetWind(windVar->GetFloat4());

    // setup emitter
    emitter->SetEmitterMeshGroupIndex(this->groupIndex);
    emitter->SetEmitterMesh(this->refMesh.get());
    emitter->SetGravity(this->gravity);
    emitter->SetTileTexture(this->tileTexture);
    emitter->SetStretchToStart(this->stretchToStart);
    emitter->SetParticleStretch(this->particleStretch);
    emitter->SetStartRotationMin(this->startRotationMin);
    emitter->SetStartRotationMax(this->startRotationMax);
    emitter->SetParticleVelocityRandomize(this->particleVelocityRandomize);
    emitter->SetParticleRotationRandomize(this->particleRotationRandomize);
    emitter->SetParticleSizeRandomize(this->particleSizeRandomize);
    emitter->SetRandomRotDir(this->randomRotDir);
    emitter->SetPrecalcTime(this->precalcTime);
    emitter->SetViewAngleFade(this->viewAngleFade);
    emitter->SetStretchDetail(this->stretchDetail);
    emitter->SetStartDelay(this->startDelay);
    emitter->SetEmitOnSurface(this->emitOnSurface);

    // set emitter settings
    emitter->SetEmissionDuration(this->emissionDuration);
    emitter->SetLooping(this->loop);
    emitter->SetActivityDistance(this->activityDistance);
    emitter->SetRenderOldestFirst(this->renderOldestFirst);
    emitter->SetIsSetup(true);

    sceneServer->SetModelTransform(matrix44());
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
nParticleShapeNode2::RenderContextCreated(nRenderContext* renderContext)
{
    nShapeNode::RenderContextCreated(renderContext);

    // see if resources need to be reloaded
    if (!this->AreResourcesValid())
    {
        this->LoadResources();
    }

    nParticle2Emitter* emitter = nParticleServer2::Instance()->NewParticleEmitter();
    n_assert(0 != emitter);

    this->emitterVarIndex = renderContext->AddLocalVar(nVariable(0, emitter));

}

//------------------------------------------------------------------------------
/**
*/
void
nParticleShapeNode2::RenderContextDestroyed(nRenderContext* renderContext)
{
    nShapeNode::RenderContextDestroyed(renderContext);

    // get emitter from render context
    nVariable& varEmitter = renderContext->GetLocalVar(this->emitterVarIndex);
    nParticle2Emitter* emitter = (nParticle2Emitter*)varEmitter.GetObj();

    n_assert(0 != emitter);
    nParticleServer2::Instance()->DeleteParticleEmitter(emitter);
}


//------------------------------------------------------------------------------
/**
    - 15-Jan-04     floh    AreResourcesValid()/LoadResource() moved to scene server
    - 28-Jan-04     daniel  emitter setup moved to RenderTransform()
*/
void
nParticleShapeNode2::Attach(nSceneServer* sceneServer, nRenderContext* renderContext)
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
nParticleShapeNode2::GetMeshUsage() const
{
    return nMesh2::ReadOnly | nMesh2::PointSprite | nMesh2::NeedsVertexShader;
}

//------------------------------------------------------------------------------
/**
*/
bool
nParticleShapeNode2::RenderGeometry(nSceneServer* sceneServer, nRenderContext* renderContext)
{
    n_assert(sceneServer);
    n_assert(renderContext);

    const nVariable& varEmitter = renderContext->GetLocalVar(this->emitterVarIndex);
    nVariable* timeVar = renderContext->GetVariable(this->timeHandle);
    n_assert2(timeVar, "No 'time' variable provided by application!");
    float curTime = timeVar->GetFloat();
    n_assert(curTime >= 0.0f);

    nParticle2Emitter* emitter = (nParticle2Emitter*)varEmitter.GetObj();
    n_assert(0 != emitter);
    if (!this->invisible)
        emitter->Render(curTime);

    return true;
}

//------------------------------------------------------------------------------
/**
    Returns the current emitter
*/
nParticle2Emitter*
nParticleShapeNode2::GetEmitter(nRenderContext* renderContext)
{
    // get emitter from render context
    nVariable& varEmitter = renderContext->GetLocalVar(this->emitterVarIndex);
    return (nParticle2Emitter*)varEmitter.GetObj();
}

