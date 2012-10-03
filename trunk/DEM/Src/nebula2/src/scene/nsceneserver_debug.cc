//------------------------------------------------------------------------------
//  scene/nsceneserver_debug.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "scene/nsceneserver.h"
#include "scene/nrendercontext.h"
#include "scene/nscenenode.h"

//------------------------------------------------------------------------------
/**
    Render a debug visualization of the light scissors.
*/
void
nSceneServer::DebugRenderLightScissors()
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();

    static const vector4 red(1.0f, 0.0f, 0.0f, 1.0f);
    static const vector4 yellow(1.0f, 1.0f, 0.0f, 1.0f);
    static const vector4 blue(0.0f, 0.0f, 1.0f, 1.0f);
    static const vector4 pink(1.0f, 0.0f, 1.0f, 1.0f);
    gfxServer->BeginLines();

    // render light scissors
    int lightIndex;
    int numLights = this->lightArray.Size();
    for (lightIndex = 0; lightIndex < numLights; lightIndex++)
    {
        const LightInfo& lightInfo = this->lightArray[lightIndex];
        const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
        const rectangle& r = lightInfo.scissorRect;
        vector2 lines[5];
        lines[0].set(r.v0.x, r.v0.y);
        lines[1].set(r.v1.x, r.v0.y);
        lines[2].set(r.v1.x, r.v1.y);
        lines[3].set(r.v0.x, r.v1.y);
        lines[4].set(r.v0.x, r.v0.y);
        if (lightGroup.renderContext->GetFlag(nRenderContext::Occluded)) gfxServer->DrawLines2d(lines, 5, blue);
        else                                                             gfxServer->DrawLines2d(lines, 5, red);
    }

    // render shadow light scissors
    int shadowLightIndex;
    int numShadowLights = this->shadowLightArray.Size();
    for (shadowLightIndex = 0; shadowLightIndex < numShadowLights; shadowLightIndex++)
    {
        const LightInfo& lightInfo = this->shadowLightArray[shadowLightIndex];
        const Group& lightGroup = this->groupArray[lightInfo.groupIndex];
        const rectangle& r = lightInfo.scissorRect;
        vector2 lines[5];
        lines[0].set(r.v0.x, r.v0.y);
        lines[1].set(r.v1.x, r.v0.y);
        lines[2].set(r.v1.x, r.v1.y);
        lines[3].set(r.v0.x, r.v1.y);
        lines[4].set(r.v0.x, r.v0.y);
        if (lightGroup.renderContext->GetFlag(nRenderContext::Occluded)) gfxServer->DrawLines2d(lines, 5, pink);
        else                                                             gfxServer->DrawLines2d(lines, 5, yellow);
    }
    gfxServer->EndLines();
}

//------------------------------------------------------------------------------
/**
    Calls RenderDebug on all shapes...
*/
void
nSceneServer::DebugRenderShapes()
{
    nGfxServer2* gfxServer = nGfxServer2::Instance();
    int i;
    int num = this->groupArray.Size();
    for (i = 0; i < num; i++)
    {
        Group& group = this->groupArray[i];
        n_assert(group.sceneNode);
        if (group.sceneNode->HasGeometry())
        {
            group.sceneNode->RenderDebug(this, group.renderContext, group.modelTransform);
        }
    }
}

//------------------------------------------------------------------------------
/**
    Renders the performance gui.
*/
void
nSceneServer::DebugRenderPerfGui()
{
	float fps = CoreSrv->GetGlobal<float>("FPS");
    int numPrimitives = CoreSrv->GetGlobal<int>("NumPrimitives");
    int numDrawCalls = CoreSrv->GetGlobal<int>("NumDrawCalls");

    nString text;
    text.Format("drawcalls: %d\t tris: %d\t fps: %f", numDrawCalls, numPrimitives, fps);
    vector4 textColor(1.0f, 0.0f, 0.0f, 1.0f);
    rectangle textRect(vector2(0.5f, 0.0f), vector2(1.0f, 1.0f));
    uint textFlags = Top | Left | NoClip | ExpandTabs;
    nGfxServer2::Instance()->DrawText(text, textColor, textRect, textFlags, false);
}
