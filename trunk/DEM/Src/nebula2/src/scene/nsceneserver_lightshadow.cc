//------------------------------------------------------------------------------
//  nsceneserver_lightshadow.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "scene/nsceneserver.h"
#include "scene/nrendercontext.h"
#include "scene/nlightnode.h"
#include "mathlib/sphere.h"
#include "util/npriorityarray.h"
//#include "shadow2/nshadowserver2.h"
#include "resource/nresourceserver.h"

//------------------------------------------------------------------------------
/**
    Return true if the given light is in the light links list of the
    shape.
*/
bool
nSceneServer::IsShapeLitByLight(const Group& shapeGroup, const Group& lightGroup)
{
    n_assert(shapeGroup.renderContext);
    n_assert(lightGroup.renderContext);

    int i;
    int num = shapeGroup.renderContext->GetNumLinks();
    for (i = 0; i < num; i++)
    {
        nRenderContext* shapeLightRenderContext = shapeGroup.renderContext->GetLinkAt(i);
        if (shapeLightRenderContext == lightGroup.renderContext)
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
    Computes the scissor rect info for a single info structure.
*/
void
nSceneServer::ComputeLightScissor(LightInfo& lightInfo)
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
    nLightNode* lightNode = (nLightNode*)lightGroup.sceneNode;
    n_assert(0 != lightNode && lightNode->IsA("nlightnode"));

    nLight::Type lightType = lightNode->GetType();
    if (nLight::Point == lightType)
    {
        // compute the point light's projected rectangle on screen
        const bbox3& localBox = lightNode->GetLocalBox();
        sphere sphere(lightGroup.modelTransform.pos_component(), localBox.extents().x);

        const matrix44& view = gfxServer->GetTransform(nGfxServer2::View);
        const matrix44& projection = gfxServer->GetTransform(nGfxServer2::Projection);
        const nCamera2& cam = gfxServer->GetCamera();

        lightInfo.scissorRect = sphere.project_screen_rh(view, projection, cam.GetNearPlane());
    }
    else if (nLight::Directional == lightType)
    {
        // directional lights cover the whole screen
        static const rectangle fullScreenRect(vector2::zero, vector2(1.0f, 1.0f));
        lightInfo.scissorRect = fullScreenRect;
    }
    else
    {
        n_error("nSceneServer::ComputeLightScissors(): unsupported light type!");
    }
}

//------------------------------------------------------------------------------
/**
    Computes the clip planes for a single light source.
*/
void
nSceneServer::ComputeLightClipPlanes(LightInfo& lightInfo)
{
    if (this->clipPlaneFencing)
    {
        nGfxServer2* gfxServer = nGfxServer2::Instance();
        const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
        nLightNode* lightNode = (nLightNode*)lightGroup.sceneNode;
        n_assert(0 != lightNode && lightNode->IsA("nlightnode"));

        nLight::Type lightType = lightNode->GetType();
        if (nLight::Point == lightType)
        {
            // get the point light's global space bounding box
            matrix44 mvp = lightGroup.modelTransform * gfxServer->GetTransform(nGfxServer2::ViewProjection);
            lightNode->GetLocalBox().get_clipplanes(mvp, lightInfo.clipPlanes);
        }
        else if (nLight::Directional == lightType)
        {
            // directional light have no user clip planes
            lightInfo.clipPlanes.Reset();
        }
        else
        {
            n_error("nSceneServer::ComputeLightClipPlanes(): unsupported light type!");
        }
    }
}

//------------------------------------------------------------------------------
/**
    Iterates through the light groups and computes the scissor rectangle
    for each light.
*/
void
nSceneServer::ComputeLightScissorsAndClipPlanes()
{
    PROFILER_START(this->profComputeScissors);
    // update lights
    int lightIndex;
    int numLights = this->lightArray.Size();
    for (lightIndex = 0; lightIndex < numLights; lightIndex++)
    {
        LightInfo& lightInfo = this->lightArray[lightIndex];
        this->ComputeLightScissor(lightInfo);
        this->ComputeLightClipPlanes(lightInfo);
    }
    PROFILER_STOP(this->profComputeScissors);
}

//------------------------------------------------------------------------------
/**
*/
void
nSceneServer::ApplyLightScissors(const LightInfo& lightInfo)
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    gfxServer->SetScissorRect(lightInfo.scissorRect);
}

//------------------------------------------------------------------------------
/**
*/
void
nSceneServer::ApplyLightClipPlanes(const LightInfo& lightInfo)
{
    if (this->clipPlaneFencing)
    {
        nGfxServer2* gfxServer = nGfxServer2::Instance();
        gfxServer->SetClipPlanes(lightInfo.clipPlanes);
    }
}

//------------------------------------------------------------------------------
/**
    Reset the light scissor rect and clip planes.
*/
void
nSceneServer::ResetLightScissorsAndClipPlanes()
{
    if (this->clipPlaneFencing)
    {
        nGfxServer2* gfxServer = nGfxServer2::Instance();
        static const rectangle fullScreenRect(vector2::zero, vector2(1.0f, 1.0f));
        gfxServer->SetScissorRect(fullScreenRect);
        nArray<plane> nullArray(0, 0);
        gfxServer->SetClipPlanes(nullArray);
    }
}

//------------------------------------------------------------------------------
/**
    Copy the current stencil buffer state into a texture color channel.
    This accumulates the stencil bits for up to 4 light sources
    (one per RGBA channel).
*/
void
nSceneServer::CopyStencilBufferToTexture(nRpPass& rpPass, const vector4& shadowLightMask)
{
    nShader2* shd = rpPass.GetShader();
    if (shd)
    {
        shd->SetVector4(nShaderState::ShadowIndex, shadowLightMask);
        nGfxServer2::Instance()->SetShader(shd);
        shd->Begin(true);
        shd->BeginPass(0);
        rpPass.DrawFullScreenQuad();
        shd->EndPass();
        shd->End();
    }
}

//------------------------------------------------------------------------------
/**
    This method goes through all attached light sources and decides which
    4 of them should cast shadows. This takes the occlusion status, distance and
    range and intensity into account. The method should be called after
    occlusion culling. The result is that the shadowLightArray will be filled.
*/
void
nSceneServer::GatherShadowLights()
{
    n_assert(this->shadowLightArray.Size() == 0);

    vector3 viewerPos = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView).pos_component();
    nPriorityArray<int> priorityArray(MaxShadowLights);

    int numLights = this->lightArray.Size();
    int lightIndex;
    for (lightIndex = 0; lightIndex < numLights; lightIndex++)
    {
        const LightInfo& lightInfo = this->lightArray[lightIndex];
        const Group& lightGroup = this->groupArray[lightInfo.groupIndex];

        // only look at shadow casting light sources
        if (lightGroup.renderContext->GetFlag(nRenderContext::CastShadows))
        {
            // ignore occluded light sources
            if (!lightGroup.renderContext->GetFlag(nRenderContext::Occluded))
            {
                nLightNode* lightNode = (nLightNode*)lightGroup.sceneNode;
                if (lightNode->GetCastShadows())
                {
                    float priority;
                    switch (lightNode->GetType())
                    {
                        case nLight::Point:
                            priority = -(lightGroup.modelTransform.pos_component() - viewerPos).len() /
                                lightNode->GetFloat(nShaderState::LightRange);
                            break;
                        case nLight::Directional:
                            priority = 100000.0f;
                            break;
                        default:
                            priority = 0.0f;
                            break;
                    }
                    priorityArray.Add(lightIndex, priority);
                }
            }
        }
    }

    // the 4 highest priority light sources are now in the priority array
    int i;
    for (i = 0; i < priorityArray.Size(); i++)
    {
        LightInfo& lightInfo = this->lightArray[priorityArray[i]];
        const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
        float shadowIntensity = lightGroup.renderContext->GetShadowIntensity();
        lightInfo.shadowLightMask = nGfxServer2::GetShadowLightIndexVector(i, shadowIntensity);
        this->shadowLightArray.Append(lightInfo);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
nSceneServer::RenderShadow(nRpPass& curPass)
{
	/*

    nShadowServer2* shadowServer = nShadowServer2::Instance();
    nGfxServer2* gfxServer = nGfxServer2::Instance();

    // check if simple or multilight shadows should be drawn
    // (DX7 can only do simple shadowing with 1 light source)
    // to 1 under DX7 and 4 under DX9
    int maxShadowLights = 1;
    if (curPass.GetDrawShadows() == nRpPass::MultiLight)
    {
        // initialize rendering for multilight shadows
        maxShadowLights = 4;
        nTexture2* renderTarget = (nTexture2*)nResourceServer::Instance()->FindResource(curPass.GetRenderTargetName(0), nResource::Texture);
        n_assert(renderTarget);
        gfxServer->SetRenderTarget(0, renderTarget);
    }
    gfxServer->SetLightingType(nGfxServer2::Off);
    gfxServer->SetHint(nGfxServer2::MvpOnly, true);

    // prepare the graphics server
    // set shadow projection matrix, this is the normal projection
    // matrix with slightly shifted near and far plane to reduce
    // z-fighting
    matrix44 shadowProj = gfxServer->GetTransform(nGfxServer2::ShadowProjection);
    gfxServer->PushTransform(nGfxServer2::Projection, shadowProj);

    if (gfxServer->BeginScene())
    {
        if (curPass.GetDrawShadows() == nRpPass::MultiLight)
        {
            gfxServer->Clear(nGfxServer2::ColorBuffer, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);
        }

        // for each shadow casting light...
        int numShadowLights = this->shadowLightArray.Size();
        if (numShadowLights > 0 && this->shadowArray.Size() > 0)
        {
            // begin shadow scene
            if (shadowServer->BeginScene())
            {
                int shadowLightIndex;
                for (shadowLightIndex = 0; shadowLightIndex < n_min(maxShadowLights, numShadowLights); shadowLightIndex++)
                {
                    // only process non-occluded lights
                    const LightInfo& lightInfo = this->shadowLightArray[shadowLightIndex];
                    Group& lightGroup = this->groupArray[lightInfo.groupIndex];
                    if (!lightGroup.renderContext->GetFlag(nRenderContext::Occluded))
                    {
                        nLightNode* lightNode = (nLightNode*)lightGroup.sceneNode;
                        n_assert(lightNode->GetCastShadows());

                        // get light position in world space
                        lightNode->ApplyLight(this, lightGroup.renderContext, lightGroup.modelTransform, lightInfo.shadowLightMask);
                        lightNode->RenderLight(this, lightGroup.renderContext, lightGroup.modelTransform);
                        const nLight& light = lightNode->GetLight();
                        float shadowIntensity = lightGroup.renderContext->GetShadowIntensity();

                        shadowServer->BeginLight(light);
                        this->ApplyLightScissors(lightInfo);

                        // FIXME: sort shadow nodes by shadow caster geometry
                        int numShapes = this->shadowArray.Size();
                        int shapeIndex;
                        for (shapeIndex = 0; shapeIndex < numShapes; shapeIndex++)
                        {
                            // render non-occluded shadow casters
                            Group& shapeGroup = this->groupArray[shadowArray[shapeIndex]];
                            n_assert(shapeGroup.renderContext->GetFlag(nRenderContext::ShadowVisible));
                            if (!shapeGroup.renderContext->GetFlag(nRenderContext::Occluded))
                            {
                                if (this->obeyLightLinks)
                                {
                                    // check if current shadow casting light sees this shape
                                    if (this->IsShapeLitByLight(shapeGroup, lightGroup))
                                    {
                                        shapeGroup.sceneNode->RenderShadow(this, shapeGroup.renderContext, shapeGroup.modelTransform);
                                        WATCHER_ADD_INT(watchNumInstances, 1);
                                    }
                                }
                                else
                                {
                                    // no obey light links, just render the shadow
                                    shapeGroup.sceneNode->RenderShadow(this, shapeGroup.renderContext, shapeGroup.modelTransform);
                                    WATCHER_ADD_INT(watchNumInstances, 1);
                                }
                            }
                        }
                        shadowServer->EndLight();

                        // if multilight shadowing, store the stencil buffer in an accumulation render target
                        if (curPass.GetDrawShadows() == nRpPass::MultiLight)
                        {
                            this->CopyStencilBufferToTexture(curPass, lightInfo.shadowLightMask);
                        }
                    }
                }
                shadowServer->EndScene();
            }
        }
        gfxServer->EndScene();
    }
    if (curPass.GetDrawShadows() == nRpPass::MultiLight)
    {
        gfxServer->SetRenderTarget(0, 0);
    }
    gfxServer->PopTransform(nGfxServer2::Projection);
    gfxServer->SetHint(nGfxServer2::MvpOnly, false);

	*/
}
