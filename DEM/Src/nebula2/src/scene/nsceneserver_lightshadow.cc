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

	nLight::Type lightType = lightNode->Light.GetType();
    if (nLight::Point == lightType)
    {
        // compute the point light's projected rectangle on screen
        const bbox3& localBox = lightNode->GetLocalBox();
        sphere sphere(lightGroup.modelTransform.pos_component(), localBox.extents().x);

        const matrix44& view = gfxServer->GetTransform(nGfxServer2::View);
        const matrix44& projection = gfxServer->GetTransform(nGfxServer2::Projection);
        lightInfo.scissorRect = sphere.project_screen_rh(view, projection, gfxServer->GetCamera().GetNearPlane());
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

		nLight::Type lightType = lightNode->Light.GetType();
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
				if (lightNode->Light.GetCastShadows())
                {
                    float priority;
                    switch (lightNode->Light.GetType())
                    {
                        case nLight::Point:
                            priority = -(lightGroup.modelTransform.pos_component() - viewerPos).len() /
								lightNode->Light.GetRange(); //GetFloat(nShaderState::LightRange);
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
