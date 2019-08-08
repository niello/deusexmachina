#pragma once
#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/ConstantBufferSet.h>
#include <Data/FixedArray.h>
#include <Data/Array.h>
#include <System/Allocators/PoolAllocator.h>

// View is a data context required to render a frame. It is defined by a scene (what to render),
// a camera (from where), render target(s) (to where), a render path (how) and some other
// parameters. NULL scene is valid and has meaning for example for GUI-only views.
// Customizable frame properties like a screen resolution, color depth, depth buffer bits
// and LOD scales are controlled and stored here.

namespace Scene
{
	class CSPS;
	class CNodeAttribute;
};

namespace UI
{
	typedef Ptr<class CUIContext> PUIContext;
};

namespace Frame
{
class CNodeAttrCamera;
class CNodeAttrAmbientLight;
typedef Ptr<class CRenderPath> PRenderPath;
typedef std::unique_ptr<class CView> PView;

enum ELODType
{
	LOD_None,				// No LOD, always selects the finest one
	LOD_Distance,			// Based on a squared distance to camera
	LOD_ScreenSizeRelative,	// Based on a relative portion of screen occupied by rendered object
	LOD_ScreenSizeAbsolute	// Based on a number of pixels occupied by rendered object
};

class CView final
{
protected:

	PRenderPath									RenderPath;
	CNodeAttrCamera*							pCamera = nullptr; //???smart ptr?

	CArray<Scene::CNodeAttribute*>				VisibilityCache;
	CArray<Render::CLightRecord>				LightCache;
	CArray<CNodeAttrAmbientLight*>				EnvironmentCache;
	bool										VisibilityCacheDirty = true; //???to flags?

	ELODType									MeshLODType = LOD_None;
	CFixedArray<float>							MeshLODScale;
	ELODType									MaterialLODType = LOD_None;
	CFixedArray<float>							MaterialLODScale;

public:

	//???add viewport settings here? to render multiple views into one RT

	//???scene start node? if NULL, render all nodes, else only that and its children
	Scene::CSPS*								pSPS = nullptr;
	UI::PUIContext								UIContext;

	Render::PGPUDriver							GPU;
	CFixedArray<Render::PRenderTarget>			RTs;
	CFixedArray<Render::PDepthStencilBuffer>	DSBuffers;
	Render::CConstantBufferSet					Globals;
	Render::PSampler							TrilinearCubeSampler; // For IBL

	CPoolAllocator<Render::CRenderNode>			RenderNodePool;
	CArray<Render::CRenderNode*>				RenderQueue;	// Cached to avoid per-frame allocations
	CArray<U16>									LightIndices;	// Cached to avoid per-frame allocations

	CView();
	~CView();

	//named/indexed texture RTs and mb named readonly system textures and named shader vars
	//!!!named resources in view bound to RP must be resolved by order number (index in array)
	//instead of looking up by name every time!
	//shadow cameras (?are generated from lights in a shadow phase?)
	//shadow map buffers (sort of RT / DS, no special case?)
	//materials for early depth, occlusion, shadows (?or in phases, predetermined?), or named materials?

	bool							SetRenderPath(CRenderPath* pNewRenderPath);
	CRenderPath*					GetRenderPath() const { return RenderPath.Get(); }
	bool							SetCamera(CNodeAttrCamera* pNewCamera);
	const CNodeAttrCamera*			GetCamera() const { return pCamera; }
	void							UpdateVisibilityCache();
	CArray<Scene::CNodeAttribute*>&	GetVisibilityCache() { return VisibilityCache; }
	CArray<Render::CLightRecord>&	GetLightCache() { return LightCache; }
	CArray<CNodeAttrAmbientLight*>&	GetEnvironmentCache() { return EnvironmentCache; }
	UPTR							GetMeshLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const;
	UPTR							GetMaterialLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const;
	bool							RequiresObjectScreenSize() const { return MeshLODType == LOD_ScreenSizeRelative || MeshLODType == LOD_ScreenSizeAbsolute || MaterialLODType == LOD_ScreenSizeRelative || MaterialLODType == LOD_ScreenSizeAbsolute; }
	bool							Render();
};

}
