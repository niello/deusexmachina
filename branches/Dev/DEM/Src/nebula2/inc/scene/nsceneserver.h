#ifndef N_SCENESERVER_H
#define N_SCENESERVER_H
//------------------------------------------------------------------------------
/**
    @class nSceneServer
    @ingroup Scene

    @brief The scene server builds a scene from nSceneNode objects and then
    renders it.

    The scene is rebuilt every frame, so some sort of culling should happen
    externally before building the scene.

    (C) 2002 RadonLabs GmbH
*/
#include "kernel/nroot.h"
#include "mathlib/matrix.h"
#include "gfx2/nshaderparams.h"
#include "util/nbucket.h"
#include "renderpath/nrenderpath2.h"
#include "gfx2/nmesh2.h"
#include "kernel/nprofiler.h"

class nRenderContext;
class nSceneNode;
class nGfxServer2;
class nShader2;
class nTexture2;
class nRpPhase;
class nOcclusionQuery;
class nMaterialNode;

//------------------------------------------------------------------------------
class nSceneServer: public nReferenced
{
public:
    /// constructor
    nSceneServer();
    /// destructor
    virtual ~nSceneServer();
    /// return pointer to instance
    static nSceneServer* Instance();
    /// set filename of render path definition XML file
    void SetRenderPathFilename(const nString& renderPathFilename);
    /// get the render path filename
    const nString& GetRenderPathFilename() const;
    /// open the scene server, call after nGfxServer2::OpenDisplay()
    virtual bool Open();
    /// close the scene server;
    virtual void Close();
    /// return true if scene server open
    bool IsOpen() const;
    /// begin the scene
    virtual bool BeginScene(const matrix44& viewer);
    /// attach the toplevel object of a scene node hierarchy to the scene
    virtual void Attach(nRenderContext* renderContext);
    /// finish the scene
    virtual void EndScene();
    /// render the scene through the default render path section
    virtual void RenderScene();
    /// present the scene
    virtual void PresentScene();
    /// begin a group node
    virtual void BeginGroup(nSceneNode* sceneNode, nRenderContext* renderContext);
    /// finish current group node
    virtual void EndGroup();
    /// set current model matrix
    void SetModelTransform(const matrix44& m);
    /// get current model matrix
    const matrix44& GetModelTransform() const;
    /// access to current render path object
    const nRenderPath2* GetRenderPath() const;
    /// enable/disable debug visualization
    void SetRenderDebug(bool b);
    /// get debug visualization flag
    bool GetRenderDebug() const;
    /// set obey light link mode
    void SetObeyLightLinks(bool b);
    /// get obey light link mode
    bool GetObeyLightLinks() const;
    /// enable/disable occlusion query
    void SetOcclusionQuery(bool b);
    /// get occlusion query status
    bool GetOcclusionQuery() const;
    /// enable/disable clip plane fencing for point lights
    void SetClipPlaneFencing(bool b);
    /// get clip plane fencing mode
    bool GetClipPlaneFencing() const;
    /// enable/disable camera node rendering
    void SetCamerasEnabled(bool b);
    /// get camera enabled flag
    bool GetCamerasEnabled() const;
    /// enable/disable standard performance gui
    void SetPerfGuiEnabled(bool b);
    /// get performance gui status
    bool GetPerfGuiEnabled() const;
    /// set the projection matrix, that should be saved model matrix
    void SaveProjectionMatrix(const matrix44& m);
    /// get the projection matrix, that was saved
    const matrix44& GetSavedProjectionMatrix() const;
private:

    static nSceneServer* Singleton;

    class Group
    {
    public:
        int parentIndex;                // index of parent in group array, or -1
        nRenderContext* renderContext;
        nSceneNode* sceneNode;
        matrix44 modelTransform;
        int lightPass;
    };

    class LightInfo
    {
    public:
        ushort groupIndex;                  // group index of the light source itself
        rectangle scissorRect;              // scissor rect of the light
        vector4 shadowLightMask;            // the shadow light index
        nArray<plane> clipPlanes;           // clip planes for the light
    };

    /// split scene nodes into light and shape nodes
    void SplitNodes();
    /// make sure scene node resources are valid
    void ValidateNodeResources();
    /// sort shape nodes for optimal rendering
    void SortNodes();
    /// static qsort() compare function for scene nodes
    static int __cdecl CompareNodes(const ushort* i1, const ushort* i2);
    /// static qsort() compare function for shadow light sources
    static int __cdecl CompareShadowLights(const LightInfo* i1, const LightInfo* i2);
    /// do the render path rendering
    void DoRenderPath(nRpSection& rpSection);
    /// render a complete phase for light mode "Off"
    void RenderPhaseLightModeOff(nRpPhase& curPhase);
    /// render a complete phase for light mode "Shader"
    void RenderPhaseLightModeShader(nRpPhase& curPhase);
    /// update scissor rectangles for one light source
    void ComputeLightScissor(LightInfo& lightInfo);
    /// update the clip planes for a single light source
    void ComputeLightClipPlanes(LightInfo& lightInfo);
    /// update scissor rectangles and clip planes once for all lights
    void ComputeLightScissorsAndClipPlanes();
    /// apply the scissor rectangle for a light group
    void ApplyLightScissors(const LightInfo& lightInfo);
    /// the clip planes for a light group
    void ApplyLightClipPlanes(const LightInfo& lightInfo);
    /// reset light scissors and clip plane
    void ResetLightScissorsAndClipPlanes();
    /// render debug visualization of light scissors
    void DebugRenderLightScissors();
    /// render debug visualization of shapes
    void DebugRenderShapes();
    /// render performance gui
    void DebugRenderPerfGui();
    /// return true if a shape's light links contain the given light
    bool IsShapeLitByLight(const Group& shapeGroup, const Group& lightGroup);
    /// render the cameras
    void RenderCameraScene();
    /// issue a single general occlusion query
    void IssueOcclusionQuery(Group& group, const vector3& viewerPos);
    /// do a general occlusion query on all root nodes
    void DoOcclusionQuery();
    /// find the N most important shadow casting light sources
    void GatherShadowLights();
    /// checks if this node is a water (with reflection, refraction cameras)
    bool IsAReflectingShape(const nMaterialNode* shapeNode)  const;
    /// calculates the distance from the bounding box of this element to viewer
    float CalculateDistanceToBoundingBox(const Group& groupNode);
    /// checks if the given shapes bounding box is visible
    bool IsShapesBBVisible(const Group& groupNode);
    /// parses the priority of this node (usually a reflection, refracting sea
    bool ParsePriority(const Group& groupNode);

    enum
    {
        MaxHierarchyDepth = 64,
        NumBuckets = 64,
        MaxShadowLights = 4,
    };

    static vector3 viewerPos;
    static int sortingOrder;

    bool isOpen;
    bool inBeginScene;
    bool obeyLightLinks;
    bool clipPlaneFencing;
    bool gfxServerInBeginScene; // HACK
    bool renderDebug;
    bool occlusionQueryEnabled;
    bool camerasEnabled;
    bool perfGuiEnabled;

    nString renderPathFilename;
    uint stackDepth;
    nRenderPath2 renderPath;

    nFixedArray<int> groupStack;
    nArray<Group> groupArray;
    nArray<LightInfo> lightArray;               // all light sources
    nArray<LightInfo> shadowLightArray;         // shadow casting light sources
    nArray<ushort> rootArray;                   // root nodes
    nArray<ushort> shadowArray;
    nArray<ushort> cameraArray;
    nBucket<ushort,NumBuckets> shapeBucket;     // contains indices of shape nodes, bucketsorted by shader

    float renderedReflectorDistance;
    nRenderContext* renderContextPtr;

    nClass* reqReflectClass;
    nClass* reqRefractClass;

    nOcclusionQuery* occlusionQuery;
    matrix44 savedProjectionMatrix;

    PROFILER_DECLARE(profFrame);
    PROFILER_DECLARE(profAttach);
    PROFILER_DECLARE(profValidateResources);
    PROFILER_DECLARE(profSplitNodes);
    PROFILER_DECLARE(profComputeScissors);
    PROFILER_DECLARE(profSortNodes);
    PROFILER_DECLARE(profRenderShadow);
    PROFILER_DECLARE(profOcclusion);
    PROFILER_DECLARE(profRenderPath);
    PROFILER_DECLARE(profRenderCameras);
};

//------------------------------------------------------------------------------
/**
*/
inline
nSceneServer*
nSceneServer::Instance()
{
    n_assert(Singleton);
    return Singleton;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSceneServer::SetPerfGuiEnabled(bool b)
{
    this->perfGuiEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSceneServer::GetPerfGuiEnabled() const
{
    return this->perfGuiEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSceneServer::SetClipPlaneFencing(bool b)
{
    this->clipPlaneFencing = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSceneServer::GetClipPlaneFencing() const
{
    return this->clipPlaneFencing;
}

//------------------------------------------------------------------------------
/**
*/
inline
const nRenderPath2*
nSceneServer::GetRenderPath() const
{
    return &this->renderPath;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSceneServer::SetRenderPathFilename(const nString& n)
{
    this->renderPath.SetFilename(n);
}

//------------------------------------------------------------------------------
/**
*/
inline
const nString&
nSceneServer::GetRenderPathFilename() const
{
    return this->renderPath.GetFilename();
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSceneServer::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
    Turn obey light links on/off. When turned off, every shape will
    be lit by every light in the scene. If turned on, the Nebula2 application
    is responsible for establishing bidirectional light links between the
    render context objects.
*/
inline
void
nSceneServer::SetObeyLightLinks(bool b)
{
    this->obeyLightLinks = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSceneServer::GetObeyLightLinks() const
{
    return this->obeyLightLinks;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSceneServer::SetModelTransform(const matrix44& m)
{
    this->groupArray.Back().modelTransform = m;
}

//------------------------------------------------------------------------------
/**
*/
inline
const matrix44&
nSceneServer::GetModelTransform() const
{
    return this->groupArray.Back().modelTransform;
}

//------------------------------------------------------------------------------
/**
    set the projectio nmatrix, that should be saved model matrix
*/
inline
void
nSceneServer::SaveProjectionMatrix(const matrix44& m)
{
    this->savedProjectionMatrix = m;
    vector3 scaleVec = vector3(2,0.5,1);
    this->savedProjectionMatrix.scale(scaleVec);
}

//------------------------------------------------------------------------------
/**
    get the projection matrix, that was saved
*/
inline
const matrix44&
nSceneServer::GetSavedProjectionMatrix() const
{
    return this->savedProjectionMatrix;
}


//------------------------------------------------------------------------------
/**
*/
inline
void
nSceneServer::SetRenderDebug(bool b)
{
    this->renderDebug = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSceneServer::GetRenderDebug() const
{
    return this->renderDebug;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSceneServer::SetOcclusionQuery(bool b)
{
    this->occlusionQueryEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSceneServer::GetOcclusionQuery() const
{
    return this->occlusionQueryEnabled;
}

//------------------------------------------------------------------------------
/**
*/
inline
void
nSceneServer::SetCamerasEnabled(bool b)
{
    this->camerasEnabled = b;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
nSceneServer::GetCamerasEnabled() const
{
    return this->camerasEnabled;
}

//------------------------------------------------------------------------------
#endif
