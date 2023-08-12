#pragma once
#include <Render/RenderFwd.h>
#include <Render/Renderer.h>
#include <Render/ShaderParamStorage.h>
#include <Render/RenderQueue.h>
#include <Events/EventsFwd.h>
#include <Data/FixedArray.h>
#include <Data/Array.h>
#include <System/Allocators/PoolAllocator.h>
#include <Math/CameraMath.h>
#include <map>

// View is a data context required to render a frame. It is defined by a scene (what to render),
// a camera (from where), render target(s) (to where), a render path (how) and some other
// parameters. Scene = nullptr is valid and may be used for GUI-only views.
// Frame view may be targeted to the swap chain or to intermediate RT(s).

namespace Scene
{
	class CSceneNode;
	class CNodeAttribute;
}

namespace Render
{
	using PLight = std::unique_ptr<class CLight>;
	using PImageBasedLight = std::unique_ptr<class CImageBasedLight>;
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
class CGraphicsScene;
class CCameraAttribute;
class CIBLAmbientLightAttribute;
class CRenderableAttribute;
typedef Ptr<class CRenderPath> PRenderPath;
typedef Ptr<class CGraphicsResourceManager> PGraphicsResourceManager;
typedef std::unique_ptr<class CView> PView;

class CView final
{
protected:

	Math::CSIMDFrustum                          _LastViewFrustum;
	acl::Vector4_32                             _ProjectionParams;
	acl::Vector4_32                             _EyePos;
	float                                       _ScreenMultiple = 0.f;


	PRenderPath									_RenderPath;
	PGraphicsResourceManager					_GraphicsMgr;
	int											_SwapChainID = INVALID_INDEX;
	CCameraAttribute*							_pCamera = nullptr; //???smart ptr?

	CGraphicsScene*								_pScene = nullptr;

	UI::PUIContext								_UIContext;
	Debug::PDebugDraw                           _DebugDraw;

	std::map<CStrID, Render::PRenderTarget>        RTs;
	std::map<CStrID, Render::PDepthStencilBuffer>  DSBuffers;
	std::vector<Render::PRenderQueueBaseT<UPTR>>   _RenderQueues;
	std::vector<Render::PRenderer>                 _Renderers;
	std::map<const Core::CRTTI*, U8>               _RenderersByRenderableType;

	std::vector<bool>                              _SpatialTreeNodeVisibility;
	std::map<UPTR, Render::PRenderable>            _Renderables;
	std::vector<decltype(_Renderables)::node_type> _RenderableNodePool;
	std::map<UPTR, Render::PLight>                 _Lights;
	std::vector<decltype(_Lights)::node_type>      _LightNodePool;

	std::map<std::pair<const Render::CEffect*, CStrID>, U32> _EffectMap;  // Source effect & input set -> index in a _ShaderTechCache
	std::vector<std::vector<const Render::CTechnique*>> _ShaderTechCache; // First index is an override index (0 is no override), second is from _EffectMap

	CArray<Render::CLightRecord>				LightCache;
	CArray<Render::CImageBasedLight*>			EnvironmentCache;

	U32                                         _SpatialTreeRebuildVersion = 0; // For spatial tree node visibility cache invalidation
	U32                                         _CameraTfmVersion = 0;

	friend class CGraphicsResourceManager;

	CView(CGraphicsResourceManager& GraphicsMgr, CStrID RenderPathID, int SwapChainID, CStrID SwapChainRTID);

	DECLARE_EVENT_HANDLER(OSWindowResized, OnOSWindowResized);

	bool UpdateCameraFrustum();
	void SynchronizeRenderables();
	void SynchronizeLights();
	void UpdateRenderables(bool ViewProjChanged);
	bool UpdateLights(bool ViewProjChanged);

public:

	//???add viewport settings here? to render multiple views into one RT

	Render::CShaderParamStorage					Globals;
	Render::PSampler							TrilinearCubeSampler; // For IBL

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
	CCameraAttribute*               CreateDefaultCamera(CStrID RenderTargetID, Scene::CSceneNode& ParentNode, bool SetAsCurrent = true);

	bool                            PrecreateRenderObjects();
	U32                             RegisterEffect(const Render::CEffect& Effect, CStrID InputSet);
	const Render::CTechnique* const* GetShaderTechCache(UPTR OverrideIndex = 0) const { return (OverrideIndex < _ShaderTechCache.size()) ? _ShaderTechCache[OverrideIndex].data() : nullptr; }
	Render::IRenderer*              GetRenderer(U8 Index) const { n_assert_dbg(Index < _Renderers.size()); return _Renderers[Index].get(); }
	Render::IRenderable*			GetRenderable(UPTR UID) { auto It = _Renderables.find(UID); return (It == _Renderables.cend()) ? nullptr : It->second.get(); }
	CArray<Render::CLightRecord>&	GetLightCache() { return LightCache; }
	auto&							GetEnvironmentCache() { return EnvironmentCache; }

	template<typename TCallback>
	DEM_FORCE_INLINE void ForEachRenderableInQueue(U32 QueueIndex, TCallback Callback)
	{
		if (QueueIndex < _RenderQueues.size() && _RenderQueues[QueueIndex])
			_RenderQueues[QueueIndex]->ForEachRenderable(Callback);
	}

	void                            Update(float dt);
	bool							Render();
	bool							Present() const;

	CRenderPath*					GetRenderPath() const { return _RenderPath.Get(); }
	bool							SetRenderTarget(CStrID ID, Render::PRenderTarget RT);
	Render::CRenderTarget*			GetRenderTarget(CStrID ID) const;
	bool							SetDepthStencilBuffer(CStrID ID, Render::PDepthStencilBuffer DS);
	Render::CDepthStencilBuffer*	GetDepthStencilBuffer(CStrID ID) const;
	void                            SetGraphicsScene(CGraphicsScene* pScene);
	CGraphicsScene*                 GetGraphicsScene() const { return _pScene; }
	void							SetCamera(CCameraAttribute* pNewCamera);
	CCameraAttribute*               GetCamera() const { return _pCamera; }
	CGraphicsResourceManager*		GetGraphicsManager() const;
	Render::CGPUDriver*				GetGPU() const;
	UI::CUIContext*                 GetUIContext() const { return _UIContext.Get(); }
	Debug::CDebugDraw*              GetDebugDrawer() const { return _DebugDraw.get(); }
	DEM::Sys::COSWindow*			GetTargetWindow() const;
	Render::PDisplayDriver			GetTargetDisplay() const;
	bool							IsFullscreen() const;
};

}
