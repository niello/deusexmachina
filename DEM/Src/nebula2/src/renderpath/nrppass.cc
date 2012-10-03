//------------------------------------------------------------------------------
//  nrppass.cc
//  (C) 2004 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "renderpath/nrppass.h"
#include "renderpath/nrprendertarget.h"
#include "renderpath/nrenderpath2.h"
#include "gfx2/ngfxserver2.h"
//#include "gui/nguiserver.h"
//#include "misc/nconserver.h"
//#include "shadow2/nshadowserver2.h"

//------------------------------------------------------------------------------
/**
*/
nRpPass::nRpPass() :
    renderPath(0),
    inBegin(false),
    rpShaderIndex(-1),
    clearFlags(0),
    clearColor(0.0f, 0.0f, 0.0f, 1.0f),
    clearDepth(1.0f),
    clearStencil(0),
    shadowTechnique(NoShadows),
    occlusionQuery(false),
    drawFullscreenQuad(false),
    drawGui(false),
    statsEnabled(true),
    shadowEnabledCondition(false),
    renderTargetNames(nGfxServer2::MaxRenderTargets)
#if __NEBULA_STATS__
    ,section(0)
#endif
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
nRpPass::~nRpPass()
{
    if (this->refQuadMesh.isvalid())
    {
        this->refQuadMesh->Release();
        this->refQuadMesh.invalidate();
    }
}

//------------------------------------------------------------------------------
/**
    Validate the pass object. This will validate the pass shader and
    also invoke Validate() on all owned phase objects.
*/
void
nRpPass::Validate()
{
    n_assert(this->renderPath);

    // setup profiler
    #if __NEBULA_STATS__
    n_assert(this->section);

    if (!this->prof.IsValid() && !this->GetDrawShadows() && !this->GetOcclusionQuery())
    {
        nString n;
        n.Format("profRpPass_%s_%s", this->section->GetName().Get(), this->name.Get());
        this->prof.Initialize(n.Get());
    }
    #endif

    // invoke validate on phases
    int i;
    int num = this->phases.Size();
    for (i = 0; i < num; i++)
    {
        this->phases[i].SetRenderPath(this->renderPath);
    #if __NEBULA_STATS__
        this->phases[i].SetSection(this->section);
        this->phases[i].SetPass(this);
    #endif
        this->phases[i].Validate();
    }

    // find shader
    if ((-1 == this->rpShaderIndex) && (!this->shaderAlias.IsEmpty()))
    {
        this->rpShaderIndex = this->renderPath->FindShaderIndex(this->shaderAlias);
        if (-1 == this->rpShaderIndex)
        {
            n_error("nRpPass::Validate(): couldn't find shader alias '%s' in render path xml file!", this->shaderAlias.Get());
        }
    }

    // validate quad mesh
    if (!this->refQuadMesh.isvalid())
    {
        nMesh2* mesh = nGfxServer2::Instance()->NewMesh("_rpmesh");
        if (!mesh->IsLoaded())
            mesh->CreateNew(4, 6, nMesh2::WriteOnly, nMesh2::Coord | nMesh2::Uv0);
        this->refQuadMesh = mesh;
    }
}

//------------------------------------------------------------------------------
/**
    Update the mesh coordinates. This takes several things into account:
    - 3d coordinates are created for direct mapping between texels and pixels
    - uv coordinates are take the render targets, or source texture's
      border color into account (hmm, tricky...)
*/
void
nRpPass::UpdateMeshCoords()
{
    // compute half pixel size for current render target
    nTexture2* renderTarget = nGfxServer2::Instance()->GetRenderTarget(0);
    int w, h;
    if (renderTarget)
    {
        w = renderTarget->GetWidth();
        h = renderTarget->GetHeight();
    }
    else
    {
        const CDisplayMode& mode = nGfxServer2::Instance()->GetDisplayMode();
        w = mode.Width;
        h = mode.Height;
    }
    vector2 pixelSize(1.0f / float(w), 1.0f / float(h));
    vector2 halfPixelSize = pixelSize * 0.5f;

    float x0 = -1.0f;
    float x1 = +1.0f - pixelSize.x;
    float y0 = -1.0f + pixelSize.y;
    float y1 = +1.0f;

    float u0 = 0.0f + halfPixelSize.x;
    float u1 = 1.0f - halfPixelSize.x;
    float v0 = 0.0f + halfPixelSize.y;
    float v1 = 1.0f - halfPixelSize.y;

    nMesh2* mesh = this->refQuadMesh;
    float* vPtr = mesh->LockVertices();
    n_assert(vPtr);
    *vPtr++ = x0; *vPtr++ = y1; *vPtr++ = 0.0f; *vPtr++ = u0; *vPtr++ = v0;
    *vPtr++ = x0; *vPtr++ = y0; *vPtr++ = 0.0f; *vPtr++ = u0; *vPtr++ = v1;
    *vPtr++ = x1; *vPtr++ = y1; *vPtr++ = 0.0f; *vPtr++ = u1; *vPtr++ = v0;
    *vPtr++ = x1; *vPtr++ = y0; *vPtr++ = 0.0f; *vPtr++ = u1; *vPtr++ = v1;
    mesh->UnlockVertices();

    ushort* iPtr = mesh->LockIndices();
    n_assert(iPtr);
    *iPtr++ = 0; *iPtr++ = 1; *iPtr++ = 2;
    *iPtr++ = 1; *iPtr++ = 3; *iPtr++ = 2;
    mesh->UnlockIndices();
}

//------------------------------------------------------------------------------
/**
    Begin a scene pass. This will set the render target, activate the pass
    shader and set any shader parameters.
*/
int
nRpPass::Begin()
{
    n_assert(!this->inBegin);
    n_assert(this->renderPath);

    #if __NEBULA_STATS__
    if (!this->GetDrawShadows() && !this->GetOcclusionQuery())
    {
        this->prof.Start();
    }
    #endif

    //// only render this pass if shadowing is enabled?
    //if (this->shadowEnabledCondition && ((nGfxServer2::Instance()->GetNumStencilBits() == 0) || (!nShadowServer2::Instance()->GetEnableShadows())))
    //{
    //    return 0;
    //}

    // gfx stats enabled?
    nGfxServer2::Instance()->SetHint(nGfxServer2::CountStats, this->statsEnabled);

    // set render targets
    int i;
    for (i = 0; i < this->renderTargetNames.Size(); i++)
    {
        // special case default render target
        if (this->renderTargetNames[i].IsEmpty())
        {
            if (0 == i)
            {
                nGfxServer2::Instance()->SetRenderTarget(i, 0);
            }
        }
        else
        {
            int renderTargetIndex = this->renderPath->FindRenderTargetIndex(this->renderTargetNames[i]);
            if (-1 == renderTargetIndex)
            {
                n_error("nRpPass: invalid render target name: %s!", this->renderTargetNames[i].Get());
            }
            nGfxServer2::Instance()->SetRenderTarget(i, this->renderPath->GetRenderTarget(renderTargetIndex).GetTexture());
        }
    }

    // invoke begin scene
    if (!nGfxServer2::Instance()->BeginScene())
    {
        return 0;
    }

    // clear render target?
    if (this->clearFlags != 0)
    {
        nGfxServer2::Instance()->Clear(this->clearFlags,
                         this->clearColor.x,
                         this->clearColor.y,
                         this->clearColor.z,
                         this->clearColor.w,
                         this->clearDepth,
                         this->clearStencil);
    }

    // apply shader (note: save/restore all shader state for pass shaders!)
    nShader2* shd = this->GetShader();
    if (shd)
    {
        this->UpdateVariableShaderParams();
        if (!this->technique.IsEmpty())
        {
            shd->SetTechnique(this->technique.Get());
        }
        shd->SetParams(this->shaderParams);
        nGfxServer2::Instance()->SetShader(shd);
        int numShaderPasses = shd->Begin(true);
        n_assert(1 == numShaderPasses); // assume 1-pass for pass shaders!
        shd->BeginPass(0);
    }

    // render GUI?
    if (this->GetDrawGui())
    {
        //nGuiServer::Instance()->Render();
        //nConServer::Instance()->Render();
    }

    // draw the full-screen quad?
    if (this->drawFullscreenQuad)
    {
        this->DrawFullScreenQuad();
    }

    this->inBegin = true;
    return this->phases.Size();
}

//------------------------------------------------------------------------------
/**
    Finish a scene pass.
*/
void
nRpPass::End()
{
    n_assert(this->renderPath);

    if (!this->inBegin)
    {
        return;
    }

    if (-1 != this->rpShaderIndex)
    {
        nShader2* shd = this->renderPath->GetShader(this->rpShaderIndex).GetShader();
        shd->EndPass();
        shd->End();
    }

    nGfxServer2::Instance()->EndScene();

    if (!this->renderTargetNames[0].IsEmpty())
    {
        for (int i=0; i < this->renderTargetNames.Size(); i++)
        {
            // Disable used render targets
            nGfxServer2::Instance()->SetRenderTarget(i, 0);
        }
    }
    this->inBegin = false;

    #if __NEBULA_STATS__
    if (!this->GetDrawShadows() && !this->GetOcclusionQuery())
    {
        this->prof.Stop();
    }
    #endif
}

//------------------------------------------------------------------------------
/**
    Renders a full-screen quad.
*/
void
nRpPass::DrawFullScreenQuad()
{
    // update the mesh coordinates
    this->UpdateMeshCoords();

    // draw the quad
    nGfxServer2::Instance()->PushTransform(nGfxServer2::Model, matrix44::identity);
    nGfxServer2::Instance()->PushTransform(nGfxServer2::View, matrix44::identity);
    nGfxServer2::Instance()->PushTransform(nGfxServer2::Projection, matrix44::identity);
    nGfxServer2::Instance()->SetMesh(this->refQuadMesh, this->refQuadMesh);
    nGfxServer2::Instance()->SetVertexRange(0, 4);
    nGfxServer2::Instance()->SetIndexRange(0, 6);
    nGfxServer2::Instance()->DrawIndexedNS(nGfxServer2::TriangleList);
    nGfxServer2::Instance()->SetMesh(0, 0);  // FIXME FLOH: find out why this is necessary! if not done mesh data will be broken...
    nGfxServer2::Instance()->PopTransform(nGfxServer2::Projection);
    nGfxServer2::Instance()->PopTransform(nGfxServer2::View);
    nGfxServer2::Instance()->PopTransform(nGfxServer2::Model);
}

//------------------------------------------------------------------------------
/**
    This gathers the current global variable values from the render path
    object and updates the shader parameter block with the new values.
*/
void
nRpPass::UpdateVariableShaderParams()
{
    // for each variable shader parameter...
    int varIndex;
    int numVars = this->varContext.GetNumVariables();
    for (varIndex = 0; varIndex < numVars; varIndex++)
    {
        const nVariable& paramVar = this->varContext.GetVariableAt(varIndex);

        // get shader state from variable
        nShaderState::Param shaderParam = (nShaderState::Param) paramVar.GetInt();

        // get the current value
        const nVariable* valueVar = nVariableServer::Instance()->GetGlobalVariable(paramVar.GetHandle());
        n_assert(valueVar);
        nShaderArg shaderArg;
        switch (valueVar->GetType())
        {
            case nVariable::Int:
                shaderArg.SetInt(valueVar->GetInt());
                break;

            case nVariable::Float:
                shaderArg.SetFloat(valueVar->GetFloat());
                break;

            case nVariable::Float4:
                shaderArg.SetFloat4(valueVar->GetFloat4());
                break;

            case nVariable::Object:
                shaderArg.SetTexture((nTexture2*) valueVar->GetObj());
                break;

            case nVariable::Matrix:
                shaderArg.SetMatrix44(&valueVar->GetMatrix());
                break;

            case nVariable::Vector4:
                shaderArg.SetVector4(valueVar->GetVector4());
                break;

            default:
                n_error("nRpPass: Invalid shader arg datatype!");
                break;
        }

        // update the shader parameter
        this->shaderParams.SetArg(shaderParam, shaderArg);
    }
}

//------------------------------------------------------------------------------
/**
*/
nShader2*
nRpPass::GetShader() const
{
    if (-1 != this->rpShaderIndex)
    {
        const nRpShader& rpShader = this->renderPath->GetShader(this->rpShaderIndex);
        nShader2* shd = rpShader.GetShader();
        return shd;
    }
    else
    {
        return 0;
    }
}

