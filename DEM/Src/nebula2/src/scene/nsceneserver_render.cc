//------------------------------------------------------------------------------
//  nsceneserver_render.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "scene/nsceneserver.h"
#include "scene/nscenenode.h"
#include "scene/nrendercontext.h"
#include "scene/nmaterialnode.h"
#include "scene/nlightnode.h"

//------------------------------------------------------------------------------
/**
    Render a complete phase for light mode "Off"
*/
void nSceneServer::RenderPhaseLightModeOff(nRpPhase& curPhase)
{
    nGfxServer2::Instance()->SetLightingType(nGfxServer2::Off);

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
        if (shapeArray.Size() > 0)
        {
            int seqNumPasses = curSeq.Begin();
            for (int seqPassIndex = 0; seqPassIndex < seqNumPasses; seqPassIndex++)
            {
                curSeq.BeginPass(seqPassIndex);

                // for each shape in bucket
                nMaterialNode* prevShapeNode = 0;
                for (int shapeIndex = 0; shapeIndex < shapeArray.Size(); shapeIndex++)
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
                        nGfxServer2::Instance()->SetTransform(nGfxServer2::Model, shapeGroup.modelTransform);

                        // set per-instance shader parameters
                        if (shaderUpdatesEnabled)
                        {
                            shapeNode->RenderShader(this, shapeGroup.renderContext);
                        }
						shapeNode->RenderGeometry(this, shapeGroup.renderContext);
						//WATCHER_ADD_INT(watchNumInstances, 1);
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

//------------------------------------------------------------------------------
/**
    Render a complete phase for light mode "Shader". This updates the light's
    status in the outer loop and then render all shapes influenced by this
    light. This will minimize render state switches between draw calls.
*/
void
nSceneServer::RenderPhaseLightModeShader(nRpPhase& curPhase)
{
    nGfxServer2::Instance()->SetLightingType(nGfxServer2::Shader);

    // for each light...
    int numLights = this->lightArray.Size();
    int lightIndex;
    for (lightIndex = 0; lightIndex < numLights; lightIndex++)
    {
        nGfxServer2::Instance()->ClearLights();
        const LightInfo& lightInfo = this->lightArray[lightIndex];
        const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
        nRenderContext* lightRenderContext = lightGroup.renderContext;
        n_assert(lightGroup.sceneNode->HasLight());
		nLightNode* pLightNode = (nLightNode*)lightGroup.sceneNode;

        // do nothing if light is occluded
        if (!lightRenderContext->GetFlag(nRenderContext::Occluded))
        {
            // apply light state
            pLightNode->ApplyLight(this, lightGroup.renderContext, lightGroup.modelTransform, lightInfo.shadowLightMask);

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
                                    nGfxServer2::Instance()->SetTransform(nGfxServer2::Model, shapeGroup.modelTransform);

                                    // set per-instance shader parameters
                                    if (shaderUpdatesEnabled) shapeNode->RenderShader(this, shapeGroup.renderContext);
                                    pLightNode->RenderLight(this, lightGroup.renderContext, lightGroup.modelTransform);

									nShader2* shd = nGfxServer2::Instance()->GetShader();
									shd->SetBool(nShaderState::AlphaBlendEnable, shapeGroup.lightPass ? true : curSeq.GetFirstLightAlphaEnabled());
									++shapeGroup.lightPass;
									shapeNode->RenderGeometry(this, shapeGroup.renderContext);
									//WATCHER_ADD_INT(watchNumInstances, 1);
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
void nSceneServer::DoRenderPath(nRpSection& rpSection)
{
    int numPasses = rpSection.Begin();
    for (int passIndex = 0; passIndex < numPasses; passIndex++)
    {
        // for each phase...
        nRpPass& curPass = rpSection.GetPass(passIndex);

        if (curPass.GetDrawShadows() != nRpPass::NoShadows)
        {
            PROFILER_START(this->profRenderShadow);
            this->GatherShadowLights();
            //this->RenderShadow(curPass);
            PROFILER_STOP(this->profRenderShadow);
        }
        else if (curPass.GetOcclusionQuery())
        {
            this->DoOcclusionQuery();
        }
        else
        {
            // default case: render phases and sequences
            int numPhases = curPass.Begin();
            for (int phaseIndex = 0; phaseIndex < numPhases; phaseIndex++)
            {
                // for each sequence...
                nRpPhase& curPhase = curPass.GetPhase(phaseIndex);
                switch (curPhase.GetLightMode())
                {
                case nRpPhase::Off:
                    this->RenderPhaseLightModeOff(curPhase);
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
