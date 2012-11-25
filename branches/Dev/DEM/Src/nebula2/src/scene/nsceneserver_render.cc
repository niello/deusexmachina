#include "scene/nsceneserver.h"
#include "scene/nrendercontext.h"
#include "scene/nmaterialnode.h"
#include "scene/nlightnode.h"
#include "renderpath/nrpphase.h"

void nSceneServer::RenderPhaseLightModeOff(nRpPhase& curPhase)
{
    nGfxServer2::Instance()->SetLightingType(nGfxServer2::Off);

    int numSeqs = curPhase.Begin();
    for (int seqIndex = 0; seqIndex < numSeqs; seqIndex++)
    {
        // check if there is anything to render for the next sequence shader at all
        nRpSequence& curSeq = curPhase.GetSequence(seqIndex);
        const nArray<ushort>& shapeArray = shapeBucket[curSeq.GetShaderBucketIndex()];
        if (!shapeArray.Size()) continue;

		int seqNumPasses = curSeq.Begin();
        for (int seqPassIndex = 0; seqPassIndex < seqNumPasses; seqPassIndex++)
        {
            curSeq.BeginPass(seqPassIndex);

            // for each shape in bucket
            nMaterialNode* prevShapeNode = NULL;
            for (int shapeIndex = 0; shapeIndex < shapeArray.Size(); shapeIndex++)
            {
                const Group& shapeGroup = this->groupArray[shapeArray[shapeIndex]];
                n_assert(shapeGroup.renderContext->GetFlag(nRenderContext::ShapeVisible));
                if (shapeGroup.renderContext->GetFlag(nRenderContext::Occluded)) continue;

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
                if (curSeq.GetShaderUpdatesEnabled()) shapeNode->RenderShader(this, shapeGroup.renderContext);
				shapeNode->RenderGeometry(this, shapeGroup.renderContext);
				//WATCHER_ADD_INT(watchNumInstances, 1);
            }
            curSeq.EndPass();
        }
        curSeq.End();
    }
    curPhase.End();
}
//---------------------------------------------------------------------

// Updates the light's status in the outer loop and then render all shapes influenced by this
// light. This will minimize render state switches between draw calls.
void nSceneServer::RenderPhaseLightModeShader(nRpPhase& curPhase)
{
    nGfxServer2::Instance()->SetLightingType(nGfxServer2::Shader);

    // for each light...
    for (int lightIndex = 0; lightIndex < lightArray.Size(); lightIndex++)
    {
        nGfxServer2::Instance()->ClearLights();
        const LightInfo& lightInfo = lightArray[lightIndex];
		const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
        n_assert(lightGroup.sceneNode->HasLight());

        // do nothing if light is occluded
        if (lightGroup.renderContext->GetFlag(nRenderContext::Occluded)) continue;

		nLightNode* pLightNode = (nLightNode*)lightGroup.sceneNode;

		// apply light state
		pLightNode->Light.SetShadowLightMask(lightInfo.shadowLightMask);
		nGfxServer2::Instance()->AddLight(pLightNode->Light, lightGroup.modelTransform);

        // now iterate through sequences...
        int numSeqs = curPhase.Begin();

        nGfxServer2::Instance()->SetScissorRect(lightInfo.scissorRect);
		if (clipPlaneFencing) nGfxServer2::Instance()->SetClipPlanes(lightInfo.clipPlanes);

        for (int seqIndex = 0; seqIndex < numSeqs; seqIndex++)
        {
            // check if there is anything to render for the next sequence shader at all
			nRpSequence& curSeq = curPhase.GetSequence(seqIndex);
			const nArray<ushort>& shapeArray = shapeBucket[curSeq.GetShaderBucketIndex()];
			if (!shapeArray.Size()) continue;

			int seqNumPasses = curSeq.Begin();
            for (int seqPassIndex = 0; seqPassIndex < seqNumPasses; seqPassIndex++)
            {
                curSeq.BeginPass(seqPassIndex);

                // for each shape in bucket
                nMaterialNode* prevShapeNode = 0;
                for (int shapeIndex = 0; shapeIndex < shapeArray.Size(); shapeIndex++)
                {
                    Group& shapeGroup = this->groupArray[shapeArray[shapeIndex]];
                    n_assert(shapeGroup.renderContext->GetFlag(nRenderContext::ShapeVisible));

                    // don't render if shape is occluded
                    if (shapeGroup.renderContext->GetFlag(nRenderContext::Occluded)) continue;

					// don't render if shape not lit by current light
                    if (obeyLightLinks && !IsShapeLitByLight(shapeGroup, lightGroup)) continue;

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
                    if (curSeq.GetShaderUpdatesEnabled()) shapeNode->RenderShader(this, shapeGroup.renderContext);
					
					//nGfxServer2::Instance()->SetTransform(nGfxServer2::Light, lightGroup.modelTransform);
					nShader2* shd = nGfxServer2::Instance()->GetShader();

					// For directional lights, the light pos shader attributes actually hold the light direction
					if (shd->IsParameterUsed(nShaderState::ModelLightPos))
					{
						const matrix44& Tfm = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvModelLight);
						const vector3& Value = (pLightNode->Light.GetType() == nLight::Directional) ? Tfm.z_component() : Tfm.pos_component();
						shd->SetVector3(nShaderState::ModelLightPos, Value);
					}

					shd->SetBool(nShaderState::AlphaBlendEnable, shapeGroup.lightPass ? true : curSeq.GetFirstLightAlphaEnabled());
					++shapeGroup.lightPass;
					shapeNode->RenderGeometry(this, shapeGroup.renderContext);
					//WATCHER_ADD_INT(watchNumInstances, 1);
                }
                curSeq.EndPass();
            }
            curSeq.End();
        }
        curPhase.End();
    }

	// Reset light scissors and clip planes
    static const rectangle fullScreenRect(vector2::zero, vector2(1.0f, 1.0f));
    nGfxServer2::Instance()->SetScissorRect(fullScreenRect);

	if (this->clipPlaneFencing)
    {
        nArray<plane> nullArray(0, 0);
        nGfxServer2::Instance()->SetClipPlanes(nullArray);
    }
}
//---------------------------------------------------------------------
