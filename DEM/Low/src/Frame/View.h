#pragma once
#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/Renderer.h>
#include <Render/ShaderParamStorage.h>
#include <Events/EventsFwd.h>
#include <Data/FixedArray.h>
#include <Data/Array.h>
#include <System/Allocators/PoolAllocator.h>
#include <map>

// View is a data context required to render a frame. It is defined by a scene (what to render),
// a camera (from where), render target(s) (to where), a render path (how) and some other
// parameters. Scene = nullptr is valid and may be used for GUI-only views.
// Frame view may be targeted to the swap chain or to intermediate RT(s).

namespace Scene
{
	class CSPS;
	class CSceneNode;
	class CNodeAttribute;
}

namespace DEM::Sys
{
	class COSWindow;
}

namespace UI
{
	typedef Ptr<class CUIContext> PUIContext;
}

namespace Debug
{
	typedef std::unique_ptr<class CDebugDraw> PDebugDraw;
}

namespace Frame
{
class CCameraAttribute;
class CAmbientLightAttribute;
class CRenderableAttribute;
typedef Ptr<class CRenderPath> PRenderPath;
typedef Ptr<class CGraphicsResourceManager> PGraphicsResourceManager;
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

	PRenderPath									_RenderPath;
	PGraphicsResourceManager					_GraphicsMgr;
	int											_SwapChainID = INVALID_INDEX;
	CCameraAttribute*							pCamera = nullptr; //???smart ptr?

	//???scene start node? if nullptr, render all nodes, else only that and its children
	Scene::CSPS*								_pSPS = nullptr;

	UI::PUIContext								_UIContext;
	Debug::PDebugDraw                           _DebugDraw;

	std::map<CStrID, Render::PRenderTarget>			RTs;
	std::map<CStrID, Render::PDepthStencilBuffer>	DSBuffers;

	std::unordered_map<const CRenderableAttribute*, Render::PRenderable> _RenderObjects;

	CArray<Scene::CNodeAttribute*>				VisibilityCache;
	CArray<Render::CLightRecord>				LightCache;
	CArray<CAmbientLightAttribute*>				EnvironmentCache;
	bool										VisibilityCacheDirty = true; //???to flags?

	ELODType									MeshLODType = LOD_None;
	CFixedArray<float>							MeshLODScale;
	ELODType									MaterialLODType = LOD_None;
	CFixedArray<float>							MaterialLODScale;

	friend class CGraphicsResourceManager;

	CView(CGraphicsResourceManager& GraphicsMgr, CStrID RenderPathID, int SwapChainID, CStrID SwapChainRTID);

	DECLARE_EVENT_HANDLER(OSWindowResized, OnOSWindowResized);

public:

	//???add viewport settings here? to render multiple views into one RT

	Render::CShaderParamStorage					Globals;
	Render::PSampler							TrilinearCubeSampler; // For IBL

	CPool<Render::CRenderNode>                  RenderNodePool;
	Render::CRenderQueue        				RenderQueue;	// Cached to avoid per-frame allocations
	CArray<U16>									LightIndices;	// Cached to avoid per-frame allocations

	~CView();

	//named/indexed texture RTs and mb named readonly system textures and named shader vars
	//!!!named resources in view bound to RP must be resolved by order number (index in array)
	//instead of looking up by name every time!
	//shadow cameras (?are generated from lights in a shadow phase?)
	//shadow map buffers (sort of RT / DS, no special case?)
	//materials for early depth, occlusion, shadows (?or in phases, predetermined?), or named materials?

	bool							CreateUIContext(CStrID RenderTargetID = CStrID::Empty);
	bool                            CreateDebugDrawer();
	bool                            CreateMatchingDepthStencilBuffer(CStrID RenderTargetID, CStrID BufferID, Render::EPixelFormat Format = Render::PixelFmt_DefaultDepthBuffer);

	bool                            PrecreateRenderObjects(Scene::CSceneNode& RootNode);
	Render::IRenderable*            GetRenderObject(const CRenderableAttribute& Attr);
	void							UpdateVisibilityCache();
	CArray<Scene::CNodeAttribute*>&	GetVisibilityCache() { return VisibilityCache; }
	CArray<Render::CLightRecord>&	GetLightCache() { return LightCache; }
	CArray<CAmbientLightAttribute*>&	GetEnvironmentCache() { return EnvironmentCache; }
	UPTR							GetMeshLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const;
	UPTR							GetMaterialLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const;
	bool							RequiresObjectScreenSize() const { return MeshLODType == LOD_ScreenSizeRelative || MeshLODType == LOD_ScreenSizeAbsolute || MaterialLODType == LOD_ScreenSizeRelative || MaterialLODType == LOD_ScreenSizeAbsolute; }

	void                            Update(float dt);
	bool							Render();
	bool							Present() const;

	CRenderPath*					GetRenderPath() const { return _RenderPath.Get(); }
	bool							SetRenderTarget(CStrID ID, Render::PRenderTarget RT);
	Render::CRenderTarget*			GetRenderTarget(CStrID ID) const;
	bool							SetDepthStencilBuffer(CStrID ID, Render::PDepthStencilBuffer DS);
	Render::CDepthStencilBuffer*	GetDepthStencilBuffer(CStrID ID) const;
	void                            SetScene(Scene::CSPS* pSPS);
	Scene::CSPS*                    GetScene() const { return _pSPS; }
	bool							SetCamera(CCameraAttribute* pNewCamera);
	CCameraAttribute*               GetCamera() const { return pCamera; }
	CGraphicsResourceManager*		GetGraphicsManager() const;
	Render::CGPUDriver*				GetGPU() const;
	UI::CUIContext*                 GetUIContext() const { return _UIContext.Get(); }
	Debug::CDebugDraw*              GetDebugDrawer() const { return _DebugDraw.get(); }
	DEM::Sys::COSWindow*			GetTargetWindow() const;
	Render::PDisplayDriver			GetTargetDisplay() const;
	bool							IsFullscreen() const;
};

}
