//------------------------------------------------------------------------------
//  nsceneserver_render.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "scene/nsceneserver.h"
#include "scene/nscenenode.h"
#include "scene/nrendercontext.h"
#include "scene/nmaterialnode.h"

//------------------------------------------------------------------------------
/**
    Render a single shape with light mode Off. Called by generic
    RenderShape() method.
*/
void
nSceneServer::RenderShapeLightModeOff(const Group& shapeGroup)
{
    this->ffpLightingApplied = false;
    shapeGroup.sceneNode->RenderGeometry(this, shapeGroup.renderContext);
    //WATCHER_ADD_INT(watchNumInstances, 1);
}

//------------------------------------------------------------------------------
/**
    Render a complete phase for light mode "Off" or "FFP"
*/
void
nSceneServer::RenderPhaseLightModeOff(nRpPhase& curPhase)
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    gfxServer->SetLightingType(nGfxServer2::Off);

    int numSeqs = curPhase.Begin();
    int seqIndex;
    for (seqIndex = 0; seqIndex < numSeqs; seqIndex++)
    {
        // check if there is anything to render for the next sequence shader at all
        nRpSequence& curSeq = curPhase.GetSequence(seqIndex);
        bool shaderUpdatesEnabled = curSeq.GetShaderUpdatesEnabled();
        int bucketIndex = curSeq.GetShaderBucketIndex();
        n_assert(bucketIndex >= 0);
        const nArray<ushort>& shapeArray = this->shapeBucket[bucketIndex];
        int numShapes = shapeArray.Size();
        if (numShapes > 0)
        {
            int seqNumPasses = curSeq.Begin();
            int seqPassIndex;
            for (seqPassIndex = 0; seqPassIndex < seqNumPasses; seqPassIndex++)
            {
                curSeq.BeginPass(seqPassIndex);

                // for each shape in bucket
                nMaterialNode* prevShapeNode = 0;
                int shapeIndex;
                for (shapeIndex = 0; shapeIndex < numShapes; shapeIndex++)
                {
                    const Group& shapeGroup = this->groupArray[shapeArray[shapeIndex]];
                    n_assert(shapeGroup.renderContext->GetFlag(nRenderContext::ShapeVisible));
                    if (!shapeGroup.renderContext->GetFlag(nRenderContext::Occluded))
                    {
                        nMaterialNode* shapeNode = (nMaterialNode*)shapeGroup.sceneNode;
                        if (shapeNode != prevShapeNode)
                        {
                            // start a new instance set
                            shapeNode->ApplyShader(this);
                            shapeNode->ApplyGeometry(this);
                            //WATCHER_ADD_INT(watchNumInstanceGroups, 1);
                        }
                        prevShapeNode = shapeNode;

                        // set modelview matrix for the shape
                        gfxServer->SetTransform(nGfxServer2::Model, shapeGroup.modelTransform);

                        // set per-instance shader parameters
                        if (shaderUpdatesEnabled)
                        {
                            shapeNode->RenderShader(this, shapeGroup.renderContext);
                        }
                        this->RenderShapeLightModeOff(shapeGroup);
                    }
                }
                curSeq.EndPass();
            }
            curSeq.End();
        }
    }
    curPhase.End();
}

//------------------------------------------------------------------------------
/**
    Render a single shape with light mode FFP (Fixed Function Pipeline).
    Called by generic RenderShape() method.

    FIXME: obey light OCCLUSION status!!!
*/
void
nSceneServer::RenderShapeLightModeFFP(const Group& shapeGroup)
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    vector4 dummyShadowLightMask;

    // Render with vertex-lighting and multiple light sources
    if (this->obeyLightLinks)
    {
        // use light links, each shape render context is linked to
        // all light render context which illuminate this shape,
        // light links are provided by the application
        gfxServer->ClearLights();
        int numLights = shapeGroup.renderContext->GetNumLinks();
        int lightIndex;
        for (lightIndex = 0; lightIndex < n_min(numLights, nGfxServer2::MaxLights); lightIndex++)
        {
            nRenderContext* lightRenderContext = shapeGroup.renderContext->GetLinkAt(lightIndex);
            const Group& lightGroup = this->groupArray[lightRenderContext->GetSceneGroupIndex()];
            n_assert(lightRenderContext == lightGroup.renderContext);
            n_assert(lightGroup.sceneNode->HasLight());
            lightGroup.sceneNode->RenderLight(this, lightGroup.renderContext, lightGroup.modelTransform);
            lightGroup.sceneNode->ApplyLight(this, lightGroup.renderContext, lightGroup.modelTransform, dummyShadowLightMask);
        }
    }
    else if (!this->ffpLightingApplied)
    {
        // ignore light links, each shape is influenced by each light
        // Optimization: if lighting has been applied for this
        // frame already, we don't need to do it again. This will only
        // work if rendering doesn't go through light links though
        gfxServer->ClearLights();
        int numLights = this->lightArray.Size();
        int lightIndex;
        for (lightIndex = 0; lightIndex < n_min(numLights, nGfxServer2::MaxLights); lightIndex++)
        {
            const Group& lightGroup = this->groupArray[this->lightArray[lightIndex].groupIndex];
            n_assert(lightGroup.sceneNode->HasLight());
            lightGroup.sceneNode->RenderLight(this, lightGroup.renderContext, lightGroup.modelTransform);
            lightGroup.sceneNode->ApplyLight(this, lightGroup.renderContext, lightGroup.modelTransform, dummyShadowLightMask);
        }
        this->ffpLightingApplied = true;
    }
    shapeGroup.sceneNode->RenderGeometry(this, shapeGroup.renderContext);
    //WATCHER_ADD_INT(watchNumInstances, 1);
}

//------------------------------------------------------------------------------
/**
    Render a complete phase for light mode "FFP", this is pretty much the same
    as LightModeOff. For each shapes, the state for all lights influencing
    this shape are set, and then the shape is rendered.
*/
void
nSceneServer::RenderPhaseLightModeFFP(nRpPhase& curPhase)
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    gfxServer->SetLightingType(nGfxServer2::FFP);

    int numSeqs = curPhase.Begin();
    int seqIndex;
    for (seqIndex = 0; seqIndex < numSeqs; seqIndex++)
    {
        // check if there is anything to render for the next sequence shader at all
        nRpSequence& curSeq = curPhase.GetSequence(seqIndex);
        bool shaderUpdatesEnabled = curSeq.GetShaderUpdatesEnabled();
        int bucketIndex = curSeq.GetShaderBucketIndex();
        n_assert(bucketIndex >= 0);
        const nArray<ushort>& shapeArray = this->shapeBucket[bucketIndex];
        int numShapes = shapeArray.Size();
        if (numShapes > 0)
        {
            int seqNumPasses = curSeq.Begin();
            int seqPassIndex;
            for (seqPassIndex = 0; seqPassIndex < seqNumPasses; seqPassIndex++)
            {
                curSeq.BeginPass(seqPassIndex);

                // for each shape in bucket
                int shapeIndex;
                nMaterialNode* prevShapeNode = 0;
                for (shapeIndex = 0; shapeIndex < numShapes; shapeIndex++)
                {
                    const Group& shapeGroup = this->groupArray[shapeArray[shapeIndex]];
                    n_assert(shapeGroup.renderContext->GetFlag(nRenderContext::ShapeVisible));
                    if (!shapeGroup.renderContext->GetFlag(nRenderContext::Occluded))
                    {
                        nMaterialNode* shapeNode = (nMaterialNode*)shapeGroup.sceneNode;
                        if (shapeNode != prevShapeNode)
                        {
                            // start a new instance set
                            shapeNode->ApplyShader(this);
                            shapeNode->ApplyGeometry(this);
                            //WATCHER_ADD_INT(watchNumInstanceGroups, 1);
                        }
                        prevShapeNode = shapeNode;

                        // set modelview matrix for the shape
                        gfxServer->SetTransform(nGfxServer2::Model, shapeGroup.modelTransform);

                        // set per-instance shader parameters
                        if (shaderUpdatesEnabled)
                        {
                            shapeNode->RenderShader(this, shapeGroup.renderContext);
                        }
                        this->RenderShapeLightModeFFP(shapeGroup);
                    }
                }
                curSeq.EndPass();
            }
            curSeq.End();
        }
    }
    curPhase.End();
}

//------------------------------------------------------------------------------
/**
    Render a single shape with light mode Shader.
    Called by generic RenderShape() method.
*/
void
nSceneServer::RenderShapeLightModeShader(Group& shapeGroup, const nRpSequence& seq)
{
    bool firstLightAlpha = seq.GetFirstLightAlphaEnabled();
    nShader2* shd = nGfxServer2::Instance()->GetShader();
    if (0 == shapeGroup.lightPass++)
    {
        shd->SetBool(nShaderState::AlphaBlendEnable, firstLightAlpha);
    }
    else
    {
        shd->SetBool(nShaderState::AlphaBlendEnable, true);
    }
    shapeGroup.sceneNode->RenderGeometry(this, shapeGroup.renderContext);
    //WATCHER_ADD_INT(watchNumInstances, 1);
}

//------------------------------------------------------------------------------
/**
    Render a complete phase for light mode "Shader". This updates the light's
    status in the outer loop and then render all shapes influenced by this
    light. This will minimize render state switches between draw calls.
*/
void
nSceneServer::RenderPhaseLightModeShader(nRpPhase& curPhase)
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    gfxServer->SetLightingType(nGfxServer2::Shader);
    this->ffpLightingApplied = false;

    // for each light...
    int numLights = this->lightArray.Size();
    int lightIndex;
    for (lightIndex = 0; lightIndex < numLights; lightIndex++)
    {
        gfxServer->ClearLights();
        const LightInfo& lightInfo = this->lightArray[lightIndex];
        const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
        nRenderContext* lightRenderContext = lightGroup.renderContext;
        n_assert(lightGroup.sceneNode->HasLight());

        // do nothing if light is occluded
        if (!lightRenderContext->GetFlag(nRenderContext::Occluded))
        {
            // apply light state
            lightGroup.sceneNode->ApplyLight(this, lightGroup.renderContext, lightGroup.modelTransform, lightInfo.shadowLightMask);

            // now iterate through sequences...
            int numSeqs = curPhase.Begin();

            // NOTE: nRpPhase::Begin resets the scissor rect, thus this must happen afterwards!
            this->ApplyLightScissors(lightInfo);
            this->ApplyLightClipPlanes(lightInfo);

            int seqIndex;
            for (seqIndex = 0; seqIndex < numSeqs; seqIndex++)
            {
                // check if there is anything to render for the next sequence shader at all
                nRpSequence& curSeq = curPhase.GetSequence(seqIndex);
                bool shaderUpdatesEnabled = curSeq.GetShaderUpdatesEnabled();
                int bucketIndex = curSeq.GetShaderBucketIndex();
                n_assert(bucketIndex >= 0);
                const nArray<ushort>& shapeArray = this->shapeBucket[bucketIndex];
                int numShapes = shapeArray.Size();
                if (numShapes > 0)
                {
                    int seqNumPasses = curSeq.Begin();
                    int seqPassIndex;
                    for (seqPassIndex = 0; seqPassIndex < seqNumPasses; seqPassIndex++)
                    {
                        curSeq.BeginPass(seqPassIndex);

                        // for each shape in bucket
                        int shapeIndex;
                        nMaterialNode* prevShapeNode = 0;
                        for (shapeIndex = 0; shapeIndex < numShapes; shapeIndex++)
                        {
                            Group& shapeGroup = this->groupArray[shapeArray[shapeIndex]];
                            n_assert(shapeGroup.renderContext->GetFlag(nRenderContext::ShapeVisible));

                            // don't render if shape is occluded
                            if (!shapeGroup.renderContext->GetFlag(nRenderContext::Occluded))
                            {
                                // don't render if shape not lit by current light
                                bool shapeInfluencedByLight = true;
                                if (this->obeyLightLinks)
                                {
                                    shapeInfluencedByLight = this->IsShapeLitByLight(shapeGroup, lightGroup);
                                }
                                if (shapeInfluencedByLight)
                                {
                                    nMaterialNode* shapeNode = (nMaterialNode*)shapeGroup.sceneNode;
                                    if (shapeNode != prevShapeNode)
                                    {
                                        // start a new instance set
                                        shapeNode->ApplyShader(this);
                                        shapeNode->ApplyGeometry(this);
                                        //WATCHER_ADD_INT(watchNumInstanceGroups, 1);
                                    }
                                    prevShapeNode = shapeNode;

                                    // set modelview matrix for the shape
                                    gfxServer->SetTransform(nGfxServer2::Model, shapeGroup.modelTransform);

                                    // set per-instance shader parameters
                                    if (shaderUpdatesEnabled)
                                    {
                                        shapeNode->RenderShader(this, shapeGroup.renderContext);
                                    }
                                    lightGroup.sceneNode->RenderLight(this, lightGroup.renderContext, lightGroup.modelTransform);

                                    this->RenderShapeLightModeShader(shapeGroup, curSeq);
                                }
                            }
                        }
                        curSeq.EndPass();
                    }
                    curSeq.End();
                }
            }
            curPhase.End();
        }
    }
    this->ResetLightScissorsAndClipPlanes();
}

//------------------------------------------------------------------------------
/**
    This implements the complete render path scene rendering. A render
    path is made of a shader hierarchy of passes, phases and sequences, designed
    to eliminate redundant shader state switches as much as possible.

    FIXME FIXME FIXME:
    Implement phase SORTING hints!
*/
void
nSceneServer::DoRenderPath(nRpSection& rpSection)
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    int numPasses = rpSection.Begin();
    int passIndex;
    for (passIndex = 0; passIndex < numPasses; passIndex++)
    {
        // for each phase...
        nRpPass& curPass = rpSection.GetPass(passIndex);

        // check if this is the GUI pass, and gui is disabled...
        if (curPass.GetDrawGui() && !this->GetGuiEnabled())
        {
            // don't render gui pass...
            continue;
        }

        if (curPass.GetDrawShadows() != nRpPass::NoShadows)
        {
            PROFILER_START(this->profRenderShadow);
            this->GatherShadowLights();
            this->RenderShadow(curPass);
            PROFILER_STOP(this->profRenderShadow);
        }
        else if (curPass.GetOcclusionQuery())
        {
            // perform light source occlusion query, this
            // marks the light sources in the scene as occluded or not
            this->DoOcclusionQuery();
        }
        else
        {
            // default case: render phases and sequences
            int numPhases = curPass.Begin();
            int phaseIndex;
            for (phaseIndex = 0; phaseIndex < numPhases; phaseIndex++)
            {
                this->ffpLightingApplied = false;

                // for each sequence...
                nRpPhase& curPhase = curPass.GetPhase(phaseIndex);
                switch (curPhase.GetLightMode())
                {
                case nRpPhase::Off:
                    this->RenderPhaseLightModeOff(curPhase);
                    break;
                case nRpPhase::FFP:
                    this->RenderPhaseLightModeFFP(curPhase);
                    break;
                case nRpPhase::Shader:
                    this->RenderPhaseLightModeShader(curPhase);
                    break;
                }
            }
            curPass.End();
        }
    }
    rpSection.End();
}
