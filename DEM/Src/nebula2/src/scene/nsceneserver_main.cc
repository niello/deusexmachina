//------------------------------------------------------------------------------
//  nsceneserver_main.cc
//  (C) 2002 RadonLabs GmbH
//------------------------------------------------------------------------------
#include "scene/nsceneserver.h"
#include "scene/nscenenode.h"
#include "scene/nrendercontext.h"
#include "scene/nmaterialnode.h"
#include "scene/nlightnode.h"
#include "gfx2/ngfxserver2.h"
#include "gfx2/nshader2.h"
#include "gfx2/nocclusionquery.h"
#include <Data/DataServer.h>
#include "resource/nresourceserver.h"
#include "mathlib/bbox.h"
#include "mathlib/sphere.h"
#include "mathlib/line.h"
#include "util/npriorityarray.h"
#include "scene/nshapenode.h"
#include "renderpath/nrpphase.h"
#include "renderpath/nrpxmlparser.h"

nSceneServer* nSceneServer::Singleton = 0;
vector3 nSceneServer::viewerPos;
int nSceneServer::sortingOrder = nRpPhase::BackToFront;

//------------------------------------------------------------------------------
/**
*/
nSceneServer::nSceneServer() :
    isOpen(false),
    inBeginScene(false),
    obeyLightLinks(false),
    gfxServerInBeginScene(false),
    renderDebug(false),
    stackDepth(0),
    shapeBucket(0, 1024),
    occlusionQuery(0),
    occlusionQueryEnabled(true),
    clipPlaneFencing(true),
    camerasEnabled(true),
    perfGuiEnabled(false)
{
    n_assert(0 == Singleton);
    Singleton = this;

    PROFILER_INIT(profFrame, "profSceneFrame");
    PROFILER_INIT(profAttach, "profSceneAttach");
    PROFILER_INIT(profValidateResources, "profSceneValidateResources");
    PROFILER_INIT(profSplitNodes, "profSceneSplitNodes");
    PROFILER_INIT(profComputeScissors, "profSceneComputeScissors");
    PROFILER_INIT(profSortNodes, "profSceneSortNodes");
    PROFILER_INIT(profRenderShadow, "profSceneRenderShadow");
    PROFILER_INIT(profOcclusion, "profSceneOcclusion");
    PROFILER_INIT(profRenderPath, "profSceneRenderPath");
    PROFILER_INIT(profRenderCameras, "profSceneRenderCameras");

    //WATCHER_INIT(watchNumInstanceGroups, "watchSceneNumInstanceGroups", DATA_TYPE(int));
    //WATCHER_INIT(watchNumInstances, "watchSceneNumInstances", DATA_TYPE(int));
    //WATCHER_INIT(watchNumOccluded, "watchSceneNumOccluded", DATA_TYPE(int));
    //WATCHER_INIT(watchNumNotOccluded, "watchSceneNumNotOccluded", DATA_TYPE(int));
    //WATCHER_INIT(watchNumPrimitives, "watchGfxNumPrimitives", DATA_TYPE(int));
    //WATCHER_INIT(watchFPS, "watchGfxFPS", DATA_TYPE(float));
    //WATCHER_INIT(watchNumDrawCalls, "watchGfxDrawCalls", DATA_TYPE(int));

    this->groupArray.SetFlags(nArray<Group>::DoubleGrowSize);
    this->lightArray.SetFlags(nArray<LightInfo>::DoubleGrowSize);
    this->shadowLightArray.SetFlags(nArray<LightInfo>::DoubleGrowSize);
    this->rootArray.SetFlags(nArray<ushort>::DoubleGrowSize);

    this->groupStack.SetSize(MaxHierarchyDepth);
    this->groupStack.Clear(0);
}

//------------------------------------------------------------------------------
/**
*/
nSceneServer::~nSceneServer()
{
    n_assert(!this->inBeginScene);
    n_assert(Singleton);
    Singleton = 0;
}

//------------------------------------------------------------------------------
/**
    Open the scene server. This will parse the render path, initialize
    the shaders assign from the render path, and finally invoke
    nGfxServer2::OpenDisplay().
*/
bool
nSceneServer::Open()
{
    n_assert(!this->isOpen);

	nRpXmlParser xmlParser;

	FrameShader.Create();
    xmlParser.SetRenderPath(FrameShader);
    if (!xmlParser.OpenXml(renderPathFilename))
    {
       n_error("nSceneServer could not open render path file '%s'!", renderPathFilename.Get());
       return false;
    }
    n_assert(FrameShader->Name.IsValid());
    n_assert(!FrameShader->shaderPath.IsEmpty());

    // initialize the shaders assign from the render path
    DataSrv->SetAssign("shaders", FrameShader->shaderPath);

    n_assert(nGfxServer2::Instance()->OpenDisplay());

    n_assert(xmlParser.ParseXml());
	xmlParser.CloseXml();

    FrameShader->Validate();

    // create an occlusion query object
    occlusionQuery = nGfxServer2::Instance()->NewOcclusionQuery();
	occlusionQuery->AddRef();

    this->isOpen = true;

	return this->isOpen;
}

//------------------------------------------------------------------------------
/**
    Close the scene server. This will also nGfxServer2::CloseDisplay().
*/
void
nSceneServer::Close()
{
    n_assert(this->isOpen);
    n_assert(this->occlusionQuery);

    this->occlusionQuery->Release();
    this->occlusionQuery = 0;

    //this->pFrameShader.Close();
	FrameShader = NULL;

//    nShadowServer2::Instance()->Close();
    nGfxServer2::Instance()->CloseDisplay();
    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
    Begin building the scene. Must be called once before attaching
    nSceneNode hierarchies using nSceneServer::Attach().

    @param  viewer      the viewer position and orientation
*/
bool
nSceneServer::BeginScene(const matrix44& invView)
{
    n_assert(this->isOpen);
    n_assert(!this->inBeginScene);

    PROFILER_START(this->profFrame);
    PROFILER_START(this->profAttach);

    this->stackDepth = 0;
    this->groupStack.Clear(0);
    this->groupArray.Reset();
    this->rootArray.Reset();

    matrix44 view = invView;
    view.invert_simple();
    nGfxServer2::Instance()->SetTransform(nGfxServer2::View, view);

    CoreSrv->SetGlobal<int>("NumInstanceGroups", 0);
    CoreSrv->SetGlobal<int>("NumInstances", 0);
    CoreSrv->SetGlobal<int>("NumOccluded", 0);
    CoreSrv->SetGlobal<int>("NumNotOccluded", 0);

    this->inBeginScene = nGfxServer2::Instance()->BeginFrame();
    return this->inBeginScene;
}

//------------------------------------------------------------------------------
/**
    Attach a scene node to the scene. This will simply invoke
    nSceneNode::Attach() on the scene node hierarchie's root object.
*/
void
nSceneServer::Attach(nRenderContext* renderContext)
{
    n_assert(renderContext);
    nSceneNode* rootNode = renderContext->GetRootNode();
    n_assert(rootNode);

    // put index of new root node on root array
    this->rootArray.Append(this->groupArray.Size());

    // reset hints in render context
    renderContext->SetFlag(nRenderContext::Occluded, false);

    // let root node hierarchy attach itself to scene
    rootNode->Attach(this, renderContext);
}

//------------------------------------------------------------------------------
/**
    Finish building the scene.
*/
void
nSceneServer::EndScene()
{
    // make sure the modelview stack is clear
    n_assert(0 == this->stackDepth);
    this->inBeginScene = false;
    PROFILER_STOP(this->profAttach);
}

//------------------------------------------------------------------------------
/**
    This method is called back by nSceneNode objects in their Attach() method
    to notify the scene server that a new hierarchy group starts.
*/
void
nSceneServer::BeginGroup(nSceneNode* sceneNode, nRenderContext* renderContext)
{
    n_assert(sceneNode);
    n_assert(renderContext);
    n_assert(this->stackDepth < MaxHierarchyDepth);

    // initialize new group node
    // FIXME: could be optimized to have no temporary
    // object which must be copied onto array
    Group group;
    group.sceneNode = sceneNode;
    group.renderContext = renderContext;
    group.lightPass = 0;
    bool isTopLevel;
    if (0 == this->stackDepth)
    {
        group.parentIndex = -1;
        isTopLevel = true;
    }
    else
    {
        group.parentIndex = this->groupStack[this->stackDepth - 1];
        isTopLevel = false;
    }
    renderContext->SetSceneGroupIndex(this->groupArray.Size());
    this->groupArray.Append(group);

    // push pointer to group onto hierarchy stack
    this->groupStack[this->stackDepth] = this->groupArray.Size() - 1;
    ++this->stackDepth;

    // immediately update node's transform
	const matrix44& ParentTfm = isTopLevel ? renderContext->GetTransform() : groupArray[group.parentIndex].modelTransform;
    ((nTransformNode*)sceneNode)->RenderTransform(this, renderContext, ParentTfm);
}

//------------------------------------------------------------------------------
/**
    This method is called back by nSceneNode objects in their Attach() method
    to notify the scene server that a hierarchy group ends.
*/
void
nSceneServer::EndGroup()
{
    n_assert(this->stackDepth > 0);
    this->stackDepth--;
}

//------------------------------------------------------------------------------
/**
    Render the actual scene. This method should be implemented by
    subclasses of nSceneServer. The frame will not be visible until
    PresentScene() is called. Additional render calls to the gfx server
    can be invoked between RenderScene() and PresentScene().
*/
void
nSceneServer::RenderScene()
{
    // split nodes into shapes and lights
    this->SplitNodes();

    // NOTE: this must happen after make sure node resources are loaded
    // because the reflection/refraction camera stuff depends on it
	// Load camera resources before other node resources
    PROFILER_START(this->profValidateResources);
    for (ushort i = 0; i < groupArray.Size(); i++)
    {
        nSceneNode* pNode = groupArray[i].sceneNode;
        if (!pNode->AreResourcesValid())
            pNode->LoadResources();
    }
    PROFILER_STOP(this->profValidateResources);

    // compute light scissor rectangles and clip planes
    PROFILER_START(this->profComputeScissors);
    for (int lightIndex = 0; lightIndex < lightArray.Size(); lightIndex++)
    {
        LightInfo& lightInfo = this->lightArray[lightIndex];
        this->ComputeLightScissor(lightInfo);
        this->ComputeLightClipPlanes(lightInfo);
    }
    PROFILER_STOP(this->profComputeScissors);

    // sort shape nodes for optimal rendering
    this->SortNodes();

    /// reset light passes in shape groups between renderpath
    for (int i = 0; i < this->shapeBucket.Size(); i++)
    {
        const nArray<ushort>& shapeArray = this->shapeBucket[i];
        for (int j = 0; j < shapeArray.Size(); j++)
            groupArray[shapeArray[j]].lightPass = 0;
    }

    // render final scene
    PROFILER_START(profRenderPath);
	// validation removed
	FrameShader->Render();
    PROFILER_STOP(profRenderPath);

    // HACK...
    this->gfxServerInBeginScene = nGfxServer2::Instance()->BeginScene();
}

//------------------------------------------------------------------------------
/**
    Finalize rendering and present the current frame. No additional rendering
    calls may be invoked after calling nSceneServer::PresentScene()
*/
void
nSceneServer::PresentScene()
{
    if (gfxServerInBeginScene)
    {
        if (renderDebug)
        {
            DebugRenderLightScissors();
            DebugRenderShapes();
        }
        if (perfGuiEnabled) DebugRenderPerfGui();
        nGfxServer2::Instance()->DrawTextBuffer();
        nGfxServer2::Instance()->EndScene();
        nGfxServer2::Instance()->PresentScene();
    }
    nGfxServer2::Instance()->EndFrame();
    PROFILER_STOP(this->profFrame);
}

//------------------------------------------------------------------------------
/**
    Split the collected scene nodes into light and shape nodes. Fills
    the lightArray[] and shapeArray[] members. This method is available
    as a convenience method for subclasses.
*/
void nSceneServer::SplitNodes()
{
    PROFILER_START(this->profSplitNodes);

    // clear arrays which are filled by this method
    this->shapeBucket.Clear();
    this->lightArray.Clear();
    this->shadowLightArray.Clear();

    for (ushort i = 0; i < groupArray.Size(); i++)
    {
        Group& group = groupArray[i];
        n_assert(group.sceneNode);

        if (group.sceneNode->HasGeometry() && group.renderContext->GetFlag(nRenderContext::ShapeVisible))
        {
            int shaderIndex = ((nMaterialNode*)group.sceneNode)->GetShaderIndex();
            if (shaderIndex > -1) shapeBucket[shaderIndex].Append(i);
        }

		if (group.sceneNode->HasLight())
        {
            group.renderContext->SetSceneLightIndex(lightArray.Size());
            LightInfo& lightInfo = *lightArray.Reserve(1);
            lightInfo.groupIndex = i;
        }
    }
    PROFILER_STOP(this->profSplitNodes);
}

//------------------------------------------------------------------------------
/**
    The scene node sorting compare function. The goal is to sort the attached
    shape nodes for optimal rendering performance.
*/
int
__cdecl
nSceneServer::CompareNodes(const ushort* i1, const ushort* i2)
{
    nSceneServer* sceneServer = nSceneServer::Singleton;
    const nSceneServer::Group& g1 = sceneServer->groupArray[*i1];
    const nSceneServer::Group& g2 = sceneServer->groupArray[*i2];

    // by render priority
    int cmp = g1.sceneNode->GetRenderPri() - g2.sceneNode->GetRenderPri();
    if (cmp != 0)
    {
        return cmp;
    }

    // by identical scene node
    cmp = int(g1.sceneNode) - int(g2.sceneNode);
    if (cmp == 0)
    {
        return cmp;
    }

    // distance to viewer (closest first)
    static vector3 dist1;
    static vector3 dist2;
    dist1.set(viewerPos.x - g1.modelTransform.M41, viewerPos.y - g1.modelTransform.M42, viewerPos.z - g1.modelTransform.M43);
    dist2.set(viewerPos.x - g2.modelTransform.M41, viewerPos.y - g2.modelTransform.M42, viewerPos.z - g2.modelTransform.M43);
    float diff = dist1.lensquared() - dist2.lensquared();

    if (sortingOrder == nRpPhase::FrontToBack)
    {
        // (closest first)
        if (diff < 0.001f)      return -1;
        if (diff > 0.001f)      return 1;
    }
    else if (sortingOrder == nRpPhase::BackToFront)
    {
        if (diff > 0.001f)      return -1;
        if (diff < 0.001f)      return 1;
    }

    // nodes are identical
    return 0;
}

//------------------------------------------------------------------------------
/**
    Specialized sorting function for shadow casting light sources,
    sorts lights by range and distance.
*/
int
__cdecl
nSceneServer::CompareShadowLights(const LightInfo* i1, const LightInfo* i2)
{
    nSceneServer* sceneServer = nSceneServer::Singleton;
    const nSceneServer::Group& g1 = sceneServer->groupArray[i1->groupIndex];
    const nSceneServer::Group& g2 = sceneServer->groupArray[i2->groupIndex];

    // compute intensity
    static vector3 dist1;
    static vector3 dist2;
    dist1.set(viewerPos.x - g1.modelTransform.M41, viewerPos.y - g1.modelTransform.M42, viewerPos.z - g1.modelTransform.M43);
    dist2.set(viewerPos.x - g2.modelTransform.M41, viewerPos.y - g2.modelTransform.M42, viewerPos.z - g2.modelTransform.M43);
    nLightNode* ln1 = (nLightNode*)g1.sceneNode;
    nLightNode* ln2 = (nLightNode*)g2.sceneNode;
    n_assert(ln1->IsA("nlightnode") && ln2->IsA("nlightnode"));
    float range1 = ln1->GetLocalBox().extents().x;
    float range2 = ln2->GetLocalBox().extents().x;
    float intensity1 = n_saturate(dist1.len() / range1);
    float intensity2 = n_saturate(dist2.len() / range2);
    float diff = intensity1 - intensity2;
    if (diff < 0.001f)      return -1;
    if (diff > 0.001f)      return 1;
    return 0;
}

//------------------------------------------------------------------------------
/**
    Sort the indices in the shape array for optimal rendering.
*/
void
nSceneServer::SortNodes()
{
    PROFILER_START(this->profSortNodes);

    // initialize the static viewer pos vector
    viewerPos = nGfxServer2::Instance()->GetTransform(nGfxServer2::InvView).pos_component();

    // for each bucket: call the sorter hook
    int i;
    int num = this->shapeBucket.Size();
    for (i = 0; i < num; i++)
    {
        int numIndices = this->shapeBucket[i].Size();
        if (numIndices > 0)
        {
            ushort* indexPtr = (ushort*)this->shapeBucket[i].Begin();
            qsort(indexPtr, numIndices, sizeof(ushort), (int(__cdecl*)(const void*, const void*))CompareNodes);
        }
    }

    // sort shadow casting light sources
    int numShadowLights = this->shadowLightArray.Size();
    if (numShadowLights > 0)
    {
        qsort(&(this->shadowLightArray[0]), numShadowLights, sizeof(LightInfo), (int(__cdecl*)(const void*, const void*))CompareShadowLights);
    }
    PROFILER_STOP(this->profSortNodes);
}
