#include "View.h"
#include <Frame/CameraAttribute.h>
#include <Frame/Lights/IBLAmbientLightAttribute.h>
#include <Frame/LightAttribute.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/RenderPath.h>
#include <Frame/GraphicsResourceManager.h>
#include <Render/Renderable.h>
#include <Render/ImageBasedLight.h>
#include <Render/RenderTarget.h>
#include <Render/ConstantBuffer.h>
#include <Render/GPUDriver.h>
#include <Render/DisplayDriver.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/Sampler.h>
#include <Render/SamplerDesc.h>
#include <Render/Effect.h>
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>
#include <Data/Algorithms.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
#include <System/OSWindow.h>
#include <System/SystemEvents.h>
#include <Core/Application.h>

namespace Frame
{

struct CBackToFrontKey32
{
	DEM_FORCE_INLINE U32 operator()(const Render::IRenderable* pRenderable) const
	{
		// Exploit the trick that we can compare valid positive floats as integers for speed-up
		// TODO C++20: use std::bit_cast
		static_assert(sizeof(float) == sizeof(U32));
		union { float f; U32 i; } Distance;
		Distance.f = pRenderable->DistanceToCamera;

		// For back to front sorting we invert values
		return std::numeric_limits<U32>().max() - Distance.i;
	}
};
//---------------------------------------------------------------------

struct CMaterialKey32
{
	DEM_FORCE_INLINE U32 operator()(const Render::IRenderable* pRenderable) const
	{
		// TODO: if (pRenderable->ChangedFlags & (Material | Mesh))

		// Sorting order:
		// 1. ( 8 bits) Tech, with all techs of the same effect being next to each other
		// 2. (12 bits) Material, with index unique inside the effect, but we are already sorted by tech and therefore by effect
		// 3. (12 bits) Primitive group, with almost guaranteed globally unique key and all groups of the same VB+IB being next to each other
		return (static_cast<U32>(pRenderable->ShaderTechKey) << 24) |
			((static_cast<U32>(pRenderable->MaterialKey) & 0xfff) << 12) |
			(static_cast<U32>(pRenderable->GeometryKey) & 0xfff);
	}
};
//---------------------------------------------------------------------

// Alpha-tested depth pre-pass should benefit from front to back sorting, especially for dense foliage,
// because Z-fail will prevent redundant pixel shader execution which is necessary for alpha testing.
// TODO PERF: on x86 we use 64-bit key because we expect the profit to be higher than sorting costs. Needs profiling.
struct CAlphaTestDepthPrePass64
{
	DEM_FORCE_INLINE U64 operator()(const Render::IRenderable* pRenderable) const
	{
		// Exploit the trick that we can compare valid positive floats as integers for speed-up
		// TODO C++20: use std::bit_cast
		static_assert(sizeof(float) == sizeof(U32));
		union { float f; U32 i; } Distance;
		Distance.f = pRenderable->DistanceToCamera;

		return (static_cast<U64>(pRenderable->ShaderTechKey) << 56) |
			((static_cast<U64>(pRenderable->MaterialKey) & 0xfff) << 44) |
			((static_cast<U64>(pRenderable->GeometryKey) & 0xfff) << 32) |
			Distance.i;
	}
};
//---------------------------------------------------------------------

CView::CView(CGraphicsResourceManager& GraphicsMgr, CStrID RenderPathID, int SwapChainID, CStrID SwapChainRTID)
	: _GraphicsMgr(&GraphicsMgr)
	, _SwapChainID(SwapChainID)
{
	// Obtain the render path resource

	_RenderPath = GraphicsMgr.GetRenderPath(RenderPathID);
	if (!_RenderPath)
	{
		::Sys::Error("CView() > no render path with ID " + RenderPathID);
		return;
	}

	// Setup global shader params, allocate storage

	auto& GlobalParams = _RenderPath->GetGlobalParamTable();
	_Globals = Render::CShaderParamStorage(GlobalParams, *GraphicsMgr.GetGPU(), true);
	for (const auto& Const : GlobalParams.GetConstants())
		_Globals.CreatePermanentConstantBuffer(Const.GetConstantBufferIndex(), Render::Access_CPU_Write | Render::Access_GPU_Read, "Globals");

	// Allocate caches for original effects and for all overrides
	_ShaderTechCache.resize(1 + _RenderPath->EffectOverrides.size());

	// Create linear cube sampler for image-based lighting
	//???FIXME: declarative in RP? as material defaults in the effect!
	Render::CSamplerDesc SamplerDesc;
	SamplerDesc.SetDefaults();
	SamplerDesc.AddressU = Render::TexAddr_Clamp;
	SamplerDesc.AddressV = Render::TexAddr_Clamp;
	SamplerDesc.AddressW = Render::TexAddr_Clamp;
	SamplerDesc.Filter = Render::TexFilter_MinMagMip_Linear;
	_TrilinearCubeSampler = GraphicsMgr.GetGPU()->CreateSampler(SamplerDesc);

	//!!!may fill with default RTs and DSs if descs are provided in RP!

	// If our view is attached to the swap chain, set the back buffer as its first RT
	if (SwapChainID >= 0 && SwapChainRTID)
	{
		SetRenderTarget(SwapChainRTID, GraphicsMgr.GetGPU()->GetSwapChainRenderTarget(SwapChainID));
		if (auto pOSWindow = GetTargetWindow())
			DISP_SUBSCRIBE_NEVENT(pOSWindow, OSWindowResized, CView, OnOSWindowResized);
	}

	// Create renderers

	// We use U8 indices for renderers
	n_assert_dbg(_RenderPath->_RendererSettings.size() <= std::numeric_limits<U8>().max());
	_Renderers.reserve(_RenderPath->_RendererSettings.size() + 1);

	// Index zero is always an invalid renderer, e.g. for unknown renderable types
	_Renderers.push_back(nullptr);

	for (const auto& RendererSettings : _RenderPath->_RendererSettings)
	{
		Render::PRenderer Renderer(static_cast<Render::IRenderer*>(RendererSettings.pRendererType->CreateClassInstance()));
		if (!Renderer || !Renderer->Init(*RendererSettings.SettingsDesc, *GraphicsMgr.GetGPU())) continue;

		for (const auto pRTTI : RendererSettings.RenderableTypes)
			_RenderersByRenderableType.emplace(pRTTI, static_cast<U8>(_Renderers.size()));

		_Renderers.push_back(std::move(Renderer));
	}

	// Create render queues used by this render path
	_RenderQueues.resize(_RenderPath->_RenderQueues.size());
	for (auto [Type, Index] : _RenderPath->_RenderQueues)
	{
//#if DEM_CPU_64
//#else
//#endif
		//???TODO: move to factory later? it is not yet well suited for templated classes.
		//!!!TODO: for x64 use 64-bit keys with more counter space for meshes and materials!
		//???need separated opaque and atest in a color phase?! atest is in render state, sorting by tech already happens.
		if (Type == "OpaqueDepthPrePass")
			_RenderQueues[Index] = std::make_unique<Render::CRenderQueue<CMaterialKey32>>(ENUM_MASK(Render::EEffectType::EffectType_Opaque));
		else if (Type == "AlphaTestDepthPrePass")
			// FIXME: also would benefit from FtB sorting here, see CAlphaTestDepthPrePass64, but can't use on 32-bit!
			// FIXME: virtual type of the queue must not depend on the key size! Now only Render::PRenderQueueBaseT_<UPTR>_ stop us from using 64 bit key!
			_RenderQueues[Index] = std::make_unique<Render::CRenderQueue<CMaterialKey32>>(ENUM_MASK(Render::EEffectType::EffectType_AlphaTest));
		else if (Type == "OpaqueMaterial")
			_RenderQueues[Index] = std::make_unique<Render::CRenderQueue<CMaterialKey32>>(ENUM_MASK(Render::EEffectType::EffectType_Opaque, Render::EEffectType::EffectType_Skybox));
		else if (Type == "AlphaTestMaterial")
			_RenderQueues[Index] = std::make_unique<Render::CRenderQueue<CMaterialKey32>>(ENUM_MASK(Render::EEffectType::EffectType_AlphaTest));
		else if (Type == "AlphaBackToFront")
			_RenderQueues[Index] = std::make_unique<Render::CRenderQueue<CBackToFrontKey32>>(ENUM_MASK(Render::EEffectType::EffectType_AlphaBlend));
		else
		{
			::Sys::Error("CView::CView() > Unknown render queue type!");
		}
	}

	_DebugName = "View " + std::to_string((size_t)this) + "(" + RenderPathID.ToString() + ")";
}
//---------------------------------------------------------------------

CView::~CView() = default;
//---------------------------------------------------------------------

// Pass empty RenderTargetID to use swap chain render target
bool CView::CreateUIContext(CStrID RenderTargetID)
{
	auto* pUI = _GraphicsMgr->GetUI();
	if (!pUI || RTs.empty()) FAIL;

	const Render::CRenderTarget* pRT = nullptr;
	const Render::CRenderTarget* pSwapChainRT = (_SwapChainID >= 0) ? _GraphicsMgr->GetGPU()->GetSwapChainRenderTarget(_SwapChainID) : nullptr;
	if (RenderTargetID)
	{
		auto It = RTs.find(RenderTargetID);
		if (It == RTs.cend() || !It->second) FAIL;
		pRT = It->second;
	}
	else if (pSwapChainRT)
	{
		pRT = pSwapChainRT;
	}
	else FAIL;

	_UIContext = pUI->CreateContext(
		static_cast<float>(pRT->GetDesc().Width),
		static_cast<float>(pRT->GetDesc().Height),
		(pRT == pSwapChainRT) ? GetTargetWindow() : nullptr);

	return _UIContext.IsValidPtr();
}
//---------------------------------------------------------------------

bool CView::CreateDebugDrawer()
{
	_DebugDraw.reset(n_new(Debug::CDebugDraw(*_GraphicsMgr)));
	return !!_DebugDraw;
}
//---------------------------------------------------------------------

bool CView::CreateMatchingDepthStencilBuffer(CStrID RenderTargetID, CStrID BufferID, Render::EPixelFormat Format)
{
	// Free the previous buffer memory before creating the new one
	SetDepthStencilBuffer(BufferID, nullptr);

	auto pTarget = GetRenderTarget(RenderTargetID);
	if (!pTarget) return false;

	const auto& RTDesc = pTarget->GetDesc();
	Render::CRenderTargetDesc DSDesc;
	DSDesc.Format = Format;
	DSDesc.MSAAQuality = Render::MSAA_None;
	DSDesc.UseAsShaderInput = false;
	DSDesc.MipLevels = 0;
	DSDesc.Width = RTDesc.Width;
	DSDesc.Height = RTDesc.Height;

	return SetDepthStencilBuffer(BufferID, _GraphicsMgr->GetGPU()->CreateDepthStencilBuffer(DSDesc));
}
//---------------------------------------------------------------------

CCameraAttribute* CView::CreateDefaultCamera(CStrID RenderTargetID, Scene::CSceneNode& ParentNode, bool SetAsCurrent)
{
	std::string CameraNodeName("_default_camera_");
	CameraNodeName += std::to_string((UPTR)this);

	float Aspect = (16.f / 9.f);
	if (auto pTarget = GetRenderTarget(RenderTargetID))
		Aspect = pTarget->GetDesc().Width / static_cast<float>(pTarget->GetDesc().Height);

	Scene::CSceneNode* pCameraNode = ParentNode.CreateChild(CStrID(CameraNodeName.c_str()), true);
	Frame::PCameraAttribute MainCamera = Frame::CCameraAttribute::CreatePerspective(Aspect);
	pCameraNode->AddAttribute(*MainCamera);
	if (SetAsCurrent) SetCamera(MainCamera);
	return MainCamera.Get();
}
//---------------------------------------------------------------------

bool CView::PrecreateRenderObjects()
{
	if (!_pScene) return false;

	SynchronizeRenderables();
	SynchronizeLights();

	//!!!TODO: update GPU resources etc for all objects including currently invisible! Rename to Preload? Make async?

	return true;
}
//---------------------------------------------------------------------

U32 CView::RegisterEffect(const Render::CEffect& Effect, CStrID InputSet)
{
	// View only tracks unique combinations of a source material and an input set. Passes may
	// apply material overrides when resolving these pairs into actual shader techniques.
	auto Key = std::make_pair(&Effect, InputSet);
	auto It = _EffectMap.find(Key);
	if (It != _EffectMap.cend()) return It->second;

	const auto NewIndex = static_cast<U32>(_EffectMap.size());
	_EffectMap.emplace(std::move(Key), NewIndex);

	// Resolve techniques for the new effect and input set
	for (UPTR ShaderTechCacheIndex = 0; ShaderTechCacheIndex < _ShaderTechCache.size(); ++ShaderTechCacheIndex)
	{
		const Render::CEffect* pEffect = &Effect;

		// Process overrides
		if (ShaderTechCacheIndex > 0)
		{
			const auto& EffectOverrides = _RenderPath->EffectOverrides[ShaderTechCacheIndex - 1];
			auto OverrideIt = EffectOverrides.find(pEffect->GetType());
			if (OverrideIt != EffectOverrides.cend())
				pEffect = _GraphicsMgr->GetEffect(OverrideIt->second).Get();
		}

		_ShaderTechCache[ShaderTechCacheIndex].push_back(pEffect ? pEffect->GetTechByInputSet(InputSet) : nullptr);
	}

	return NewIndex;
}
//---------------------------------------------------------------------

void CView::Update(float dt)
{
	ZoneScoped;

	if (_GraphicsMgr) _GraphicsMgr->Update(dt);
	if (_UIContext) _UIContext->Update(dt);
}
//---------------------------------------------------------------------

// Returns true if the frustum changed
bool CView::UpdateCameraFrustum()
{
	bool ViewProjChanged = false;

	// View changes are detected easily with a camera node transform version
	const auto CameraTfmVersion = _pCamera->GetNode()->GetTransformVersion();
	if (_CameraTfmVersion != CameraTfmVersion)
	{
		ViewProjChanged = true;
		_CameraTfmVersion = CameraTfmVersion;
		_EyePos = _pCamera->GetPosition();
	}

	// Both perspective and orthographic projections are characterized by just few matrix elements, compare only them
	const auto& Proj = _pCamera->GetProjMatrix();
	const float m00 = rtm::vector_get_x(Proj.x_axis);
	const float m11 = rtm::vector_get_y(Proj.y_axis);
	const float m22 = rtm::vector_get_z(Proj.z_axis);
	const float m32 = rtm::vector_get_z(Proj.w_axis);
	const auto ProjectionParams = rtm::vector_set(m00, m11, m22, m32);
	if (!rtm::vector_all_near_equal(_ProjectionParams, ProjectionParams))
	{
		ViewProjChanged = true;
		_ProjectionParams = ProjectionParams;
		_ScreenMultiple = 0.5f * std::max(m00, m11);
	}

	// NB: must be normalized for correct sphere culling
	if (ViewProjChanged)
		_LastViewFrustum = Math::NormalizeFrustum(Math::CalcFrustumParams(_pCamera->GetViewProjMatrix()));

	return ViewProjChanged;
}
//---------------------------------------------------------------------

void CView::SynchronizeRenderables()
{
	ZoneScoped;

	// Synchronize scene objects with their renderable mirrors
	DEM::Algo::SortedUnion(_pScene->GetRenderables(), _Renderables, [](const auto& a, const auto& b) { return a.first < b.first; },
		[this](auto ItSceneObject, auto ItViewObject)
	{
		if (ItSceneObject == _pScene->GetRenderables().cend())
		{
			// An object was removed from a scene, remove its renderable from queues and from a sync list
			if (ItViewObject->second->IsVisible)
				for (auto& Queue : _RenderQueues)
					Queue->Remove(ItViewObject->second.get());

			// NB: erasing a map doesn't affect other iterators, and DEM::Algo::SortedUnion already cached the next one, so nothing will break
			ItViewObject->second.reset();
			_RenderableNodePool.push_back(_Renderables.extract(ItViewObject));
		}
		else if (ItViewObject == _Renderables.cend())
		{
			// A new object in a scene
			const auto UID = ItSceneObject->first;
			const auto pAttr = static_cast<CRenderableAttribute*>(ItSceneObject->second.pAttr);

			// UIDs always grow (unless overflowed), and therefore adding to the end is always the right hint which gives us O(1) insertion
			//???!!!could compact UIDs when close to overflow! Do in SPS and broadcast changes OldUID->NewUID to all views. With guarantee of
			//doing this in order, we can keep an iterator and avoid logarithmic searches for each change!
			if (_RenderableNodePool.empty())
			{
				ItViewObject = _Renderables.emplace_hint(_Renderables.cend(), UID, pAttr->CreateRenderable());
			}
			else
			{
				auto& Node = _RenderableNodePool.back();
				Node.key() = UID;
				Node.mapped() = pAttr->CreateRenderable();
				ItViewObject = _Renderables.insert(_Renderables.cend(), std::move(Node));
				_RenderableNodePool.pop_back();
			}

			// Find and assign a renderer for this type of renderable
			auto ItRenderer = _RenderersByRenderableType.find(ItViewObject->second->GetRTTI());
			ItViewObject->second->RendererIndex = (ItRenderer != _RenderersByRenderableType.cend()) ? ItRenderer->second : 0;
		}
	});
}
//---------------------------------------------------------------------

void CView::SynchronizeLights()
{
	ZoneScoped;

	// Synchronize scene lights with their GPU mirrors
	DEM::Algo::SortedUnion(_pScene->GetLights(), _Lights, [](const auto& a, const auto& b) { return a.first < b.first; },
		[this](auto ItSceneObject, auto ItViewObject)
	{
		if (ItSceneObject == _pScene->GetLights().cend())
		{
			// An attribute was removed from a scene, remove its associated GPU light instance and free its GPU index.
			// CGraphicsScene::RemoveLight has already incremented ObjectLightIntersectionsVersion for all renderables
			// that were affected by this light, so CLight pointers will be cleaned up before renderers could access them.
			if (ItViewObject->second->GPUIndex != INVALID_INDEX_T<U32>)
			{
				// TODO: support freeing slots after other light types
				n_assert_dbg(ItViewObject->second->BoundsVersion && ItViewObject->second->GPUData.Type != Render::ELightType::IBL);
				_FreeLightGPUIndices.push_back(ItViewObject->second->GPUIndex);
			}
			ItViewObject->second.reset();

			// NB: erasing from an std::map doesn't affect other iterators, and SortedUnion already cached the next one
			_LightNodePool.push_back(_Lights.extract(ItViewObject));
		}
		else if (ItViewObject == _Lights.cend())
		{
			// A new object in a scene
			const auto UID = ItSceneObject->first;

			auto pAttr = static_cast<CLightAttribute*>(ItSceneObject->second.pAttr);

			// UIDs always grow (unless overflowed), and therefore adding to the end is always the right hint which gives us O(1) insertion
			//???!!!could compact UIDs when close to overflow! Do in SPS and broadcast changes OldUID->NewUID to all views. With guarantee of
			//doing this in order, we can keep an iterator and avoid logarithmic searches for each change!
			// TODO: wrap into a template method? Or inherit from map? CRTP to mix in an extract-cache to all supporting collections?!
			// TODO PERF: measure performance gain from using extract-cache!
			if (_LightNodePool.empty())
			{
				_Lights.emplace_hint(_Lights.cend(), UID, pAttr->CreateLight());
			}
			else
			{
				auto& Node = _LightNodePool.back();
				Node.key() = UID;
				Node.mapped() = pAttr->CreateLight();
				_Lights.insert(_Lights.cend(), std::move(Node));
				_LightNodePool.pop_back();
			}
		}
	});
}
//---------------------------------------------------------------------

void CView::UpdateRenderables(bool ViewProjChanged)
{
	ZoneScoped;

	// Iterate synchronized collections side by side
	auto ItSceneObject = _pScene->GetRenderables().cbegin();
	auto ItViewObject = _Renderables.cbegin();
	for (; ItViewObject != _Renderables.cend(); ++ItSceneObject, ++ItViewObject)
	{
		const CGraphicsScene::CSpatialRecord& Record = ItSceneObject->second;
		Render::IRenderable* pRenderable = ItViewObject->second.get();
		auto pAttr = static_cast<CRenderableAttribute*>(Record.pAttr);
		const bool WasVisible = pRenderable->IsVisible;

		if (!Record.BoundsVersion)
		{
			// Objects with invalid bounds are always visible. E.g. skybox. LOD is not applicable.
			pRenderable->IsVisible = true;
			pRenderable->BoundsVersion = Record.BoundsVersion;
		}
		else if (ViewProjChanged || pRenderable->BoundsVersion != Record.BoundsVersion)
		{
			//???TODO: need to test for zero bounds and set invisible to save resources on further processing?!

			const bool NoTreeNode = (Record.NodeIndex == NO_SPATIAL_TREE_NODE);
			if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2]) // Check if node has a visible part
			{
				if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2 + 1]) // Check if node has an invisible part
				{
					pRenderable->IsVisible = Math::HasIntersection(Record.Box.Center, Record.Box.Extent, _LastViewFrustum);
				}
				else pRenderable->IsVisible = true;
			}
			else pRenderable->IsVisible = false;

			pRenderable->BoundsVersion = Record.BoundsVersion;

			// Calculate LOD prerequisites. They depend only on camera and object bounds.
			if (pRenderable->IsVisible)
			{
				pRenderable->DistanceToCamera = std::sqrtf(Math::SqDistancePointAABB(_EyePos, Record.Box.Center, Record.Box.Extent));
				pRenderable->RelScreenRadius = _ScreenMultiple * rtm::vector_get_w(Record.Sphere) / std::max<float>(rtm::vector_distance3(_EyePos, Record.Box.Center), 1.0f);
			}
		}

		// Update a renderable from its scene attribute. NB: IsVisible may be set to false inside.
		if (pRenderable->IsVisible)
			pAttr->UpdateRenderable(*this, *pRenderable, ViewProjChanged);

		// Handle object visibility change
		if (WasVisible != pRenderable->IsVisible)
		{
			if (WasVisible)
			{
				for (auto& Queue : _RenderQueues)
					Queue->Remove(pRenderable);
			}
			else
			{
				for (auto& Queue : _RenderQueues)
					Queue->Add(pRenderable);
			}
		}

		// Setup and perform object-light intersection tracking
		// TODO: for pairs of static object and static light might use lightmapping or other optimizations. Exploit physics lib's collision?
		// TODO: _IsForwardLighting OR shadows / alpha etc, what needs intersections in deferred lighting
		// TODO: don't search for intersection for unlit materials if not needed for shadow
		// TODO: check pAttr->GetLightTrackingFlags()? must be > 0, but this is always true now for nonzero BoundsVersion
		const bool TrackObjectLightIntersections = pRenderable->IsVisible && Record.BoundsVersion && _RenderPath->_IsForwardLighting;
		if (TrackObjectLightIntersections != pRenderable->TrackObjectLightIntersections)
		{
			_pScene->TrackObjectLightIntersections(*pAttr, TrackObjectLightIntersections);
			pRenderable->TrackObjectLightIntersections = TrackObjectLightIntersections;
		}
		if (TrackObjectLightIntersections)
		{
			_pScene->UpdateObjectLightIntersections(*pAttr);

			// UpdateLights was already executed, and now all light contacts for this renderable are finally actual
			if (pRenderable->ObjectLightIntersectionsVersion != Record.ObjectLightIntersectionsVersion)
			{
				// Update scene level light state specific for the renderable. E.g. terrain caches lights per patch.
				// Currently the renderable itself should track if actual changes happened and the cache must be updated.
				// NB: if it fails to check properly, each view will redundantly trigger scene level cache update!
				pAttr->OnLightIntersectionsUpdated();

				pAttr->UpdateLightList(*this, *pRenderable, Record.pObjectLightIntersections);
				pRenderable->ObjectLightIntersectionsVersion = Record.ObjectLightIntersectionsVersion;
			}
		}
	}
}
//---------------------------------------------------------------------

void CView::UpdateLights(bool ViewProjChanged)
{
	ZoneScoped;

	// Just a buffer for change detection inside a loop
	Render::CGPULightInfo PrevInfo;

	// Iterate synchronized collections side by side
	auto ItSceneObject = _pScene->GetLights().cbegin();
	auto ItViewObject = _Lights.cbegin();
	for (; ItViewObject != _Lights.cend(); ++ItSceneObject, ++ItViewObject)
	{
		const CGraphicsScene::CSpatialRecord& Record = ItSceneObject->second;
		Render::CLight* pLight = ItViewObject->second.get();
		auto pAttr = static_cast<CLightAttribute*>(Record.pAttr);
		const bool WasVisible = pLight->IsVisible; // NB: new light views are created with 'false'

		if (!Record.BoundsVersion)
		{
			// Lights with invalid bounds are global and therefore always visible
			pLight->IsVisible = true;
			pLight->BoundsVersion = Record.BoundsVersion;
		}
		else if (ViewProjChanged || pLight->BoundsVersion != Record.BoundsVersion)
		{
			//???TODO: need to test for zero bounds (BoxExtent any of xyz <= 0.f) and set invisible to save resources on further processing?! But how often can expect empty bounds?

			const bool NoTreeNode = (Record.NodeIndex == NO_SPATIAL_TREE_NODE);
			if (!NoTreeNode && !_SpatialTreeNodeVisibility[Record.NodeIndex * 2]) // Check if spatial tree node is completely outside a view
				pLight->IsVisible = false;			
			else if (Math::DistancePointSphere(_EyePos, Record.Sphere) > pAttr->GetMaxDistance()) // Light sources that are too far away are considered invisible
				pLight->IsVisible = false;
			else if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2 + 1]) // Check if spatial tree node has an invisible part
				pLight->IsVisible = Math::HasIntersection(Record.Sphere, _LastViewFrustum);
			else
				pLight->IsVisible = true;

			pLight->BoundsVersion = Record.BoundsVersion;
		}

		// Update light view data and detect changes
		if (pLight->IsVisible)
		{
			// TODO PERF: this becomes really useful only with partial CB update capability which comes with D3D 11.1!
			const bool TrackChanges = WasVisible && pLight->BoundsVersion && !pLight->GPUDirty;
			if (TrackChanges) std::memcpy(&PrevInfo, &pLight->GPUData, sizeof(PrevInfo));

			pAttr->UpdateLight(*_GraphicsMgr, *pLight);

			if (TrackChanges && pLight->IsVisible && !pLight->GPUDirty && std::memcmp(&PrevInfo, &pLight->GPUData, sizeof(PrevInfo)))
				pLight->GPUDirty = true;
		}

		// Invisible local lights must free their GPU indices. Global lights can't become invisible.
		// NB: index freeing is done before UploadLightsToGPU, so all free indices are available for new lights to use.
		if (!pLight->IsVisible && pLight->GPUIndex != INVALID_INDEX_T<U32>)
		{
			// TODO: add support for local IBL (pLight->GPUData.Type == Render::ELightType::IBL)
			_FreeLightGPUIndices.push_back(pLight->GPUIndex);
			pLight->GPUIndex = INVALID_INDEX_T<U32>;
		}

		// Setup and perform object-light intersection tracking
		// TODO: for pairs of static object + static light might use lightmapping or other optimizations. Exploit physics lib's collision? Or calc once? Or preprocess offline?
		// TODO: _IsForwardLighting OR shadows / alpha etc, what needs intersections in deferred lighting
		const bool TrackObjectLightIntersections = pLight->IsVisible && Record.BoundsVersion && _RenderPath->_IsForwardLighting;
		if (TrackObjectLightIntersections != pLight->TrackObjectLightIntersections)
		{
			_pScene->TrackObjectLightIntersections(*pAttr, TrackObjectLightIntersections);
			pLight->TrackObjectLightIntersections = TrackObjectLightIntersections;
		}
		if (TrackObjectLightIntersections) _pScene->UpdateObjectLightIntersections(*pAttr);
	}

	// Update lights on GPU
	UploadLightsToGPU();
}
//---------------------------------------------------------------------

// TODO: add hard int priority
static inline bool IsLightPriorityLess(const Render::CLight* a, const Render::CLight* b)
{
	return (a->GPUData.Color.x + a->GPUData.Color.y + a->GPUData.Color.z) < (b->GPUData.Color.x + b->GPUData.Color.y + b->GPUData.Color.z);
}
//---------------------------------------------------------------------

void CView::UploadLightsToGPU()
{
	ZoneScoped;

	U32 MaxGlobalLights = 0;
	Render::CShaderConstantParam GlobalLightElm;
	if (_RenderPath->ConstGlobalLights)
	{
		MaxGlobalLights = _RenderPath->ConstGlobalLights.GetElementCount();
		GlobalLightElm = _RenderPath->ConstGlobalLights[0];
	}

	U32 MaxLocalLights = 0;
	Render::CShaderConstantParam LocalLightElm;
	if (_RenderPath->ConstLightBuffer)
	{
		//!!!for a structured buffer, max count may be not applicable! must then use the same value
		//as was used to allocate structured buffer instance!
		// e.g. see https://www.gamedev.net/forums/topic/709796-working-with-structuredbuffer-in-hlsl-directx-11/
		// StructuredBuffer<Light> lights : register(t9);
		MaxLocalLights = _RenderPath->ConstLightBuffer.GetElementCount();
		n_assert_dbg(MaxLocalLights > 0);
		n_assert_dbg(_RenderPath->ConstLightBuffer.GetElementStride() == sizeof(Render::CGPULightInfo));
		LocalLightElm = _RenderPath->ConstLightBuffer[0];
	}

	_pGlobalAmbientLight = nullptr;

	const auto PrevGlobalLightCount = _GlobalLights.size();
	_GlobalLights.clear();

	for (const auto& [UID, Light] : _Lights)
	{
		if (!Light->IsVisible) continue;

		if (Light->GPUData.Type == Render::ELightType::IBL)
		{
			// Setup IBL (ambient cubemaps)
			// TODO: later can implement local weight-blended parallax-corrected cubemaps selected by COI
			// https://seblagarde.wordpress.com/2012/09/29/image-based-lighting-approaches-and-parallax-corrected-cubemap/
			if (!_pGlobalAmbientLight)
			{
				//!!!TODO: for local IBLs find closest one (or several for blending) per instance in a renderer, fall back to global if not found!
				n_assert_dbg(!Light->BoundsVersion);

				_pGlobalAmbientLight = static_cast<Render::CImageBasedLight*>(Light.get());
				Light->GPUIndex = 0;
			}
		}
		else if (!Light->BoundsVersion)
		{
			// Global (dir) lights
			// TODO: to separate compact GPU structures, but for CPU still use GPUData?
			n_assert_dbg(Light->GPUData.Type == Render::ELightType::Directional);

			// Skip global light if not supported
			if (!MaxGlobalLights) continue;

			if (_GlobalLights.size() < MaxGlobalLights)
			{
				_GlobalLights.push_back(Light.get());
			}
			else
			{
				// Supported global light limit reached, choose highest priority lights.
				// NB: linear search and priority recalculation is OK here because there will be not more than 4-8 global lights.
				auto It = std::min_element(_GlobalLights.begin(), _GlobalLights.end(), IsLightPriorityLess);
				if (IsLightPriorityLess(*It, Light.get()))
					*It = Light.get();
			}
		}
		else // analytical local lights
		{
			if (Light->GPUIndex == INVALID_INDEX_T<U32>)
			{
				if (!_FreeLightGPUIndices.empty())
				{
					Light->GPUIndex = _FreeLightGPUIndices.back();
					_FreeLightGPUIndices.pop_back();
				}
				else if (_NextUnusedLightGPUIndex < MaxLocalLights)
					Light->GPUIndex = _NextUnusedLightGPUIndex++;
				else
					continue; // no slots left, skip this light

				Light->GPUDirty = true;
			}

			if (Light->GPUDirty)
			{
				LocalLightElm.Shift(_RenderPath->ConstLightBuffer, Light->GPUIndex);
				_Globals.SetRawConstant(LocalLightElm, Light->GPUData);
				Light->GPUDirty = false;
			}
		}
	}

	// Upload global lights
	if (MaxGlobalLights)
	{
		const auto GlobalLightCount = _GlobalLights.size();
		if (GlobalLightCount)
		{
			// Require sorting for the algorithm below to work
			std::sort(_GlobalLights.begin(), _GlobalLights.end(), [](const Render::CLight* a, const Render::CLight* b) { return a->GPUIndex < b->GPUIndex; });

			// Fill the GPU buffer preserving already uploaded lights in their positions when possible
			auto NextForCheck = 0;
			auto NextForMove = GlobalLightCount - 1;
			for (size_t GPUIndex = 0; GPUIndex < GlobalLightCount; ++GPUIndex)
			{
				Render::CLight* pLight = _GlobalLights[NextForCheck];
				if (pLight->GPUIndex != GPUIndex)
				{
					if (NextForCheck != NextForMove) pLight = _GlobalLights[NextForMove--];
					pLight->GPUIndex = GPUIndex;
					pLight->GPUDirty = true;
				}
				else
				{
					++NextForCheck;
				}

				if (pLight->GPUDirty)
				{
					GlobalLightElm.Shift(_RenderPath->ConstGlobalLights, GPUIndex);
					_Globals.SetRawConstant(GlobalLightElm, pLight->GPUData);
					pLight->GPUDirty = false;
				}
			}
		}

		// Terminate global light buffer to skip unfilled slots when calculating lighting
		if (GlobalLightCount < MaxGlobalLights)
		{
			// Fake element for correct prev count tracking
			_GlobalLights.push_back(nullptr);

			if (PrevGlobalLightCount != _GlobalLights.size())
			{
				//???TODO: write only invalid light type instead of the whole invalid light data?
				static const Render::CGPULightInfo InvalidGlobalLight{};
				GlobalLightElm.Shift(_RenderPath->ConstGlobalLights, GlobalLightCount);
				_Globals.SetRawConstant(GlobalLightElm, InvalidGlobalLight);
			}
		}
	}
}
//---------------------------------------------------------------------

// Everything might be unbound since the previous frame, so bind even if values didn't change
void CView::ApplyGlobalShaderParams()
{
	ZoneScoped;

	Render::CGPUDriver* pGPU = GetGPU();

	_Globals.Apply();

	if (_pGlobalAmbientLight)
	{
		if (_RenderPath->RsrcIrradianceMap)
			_RenderPath->RsrcIrradianceMap->Apply(*pGPU, _pGlobalAmbientLight->_IrradianceMap);

		if (_RenderPath->RsrcRadianceEnvMap)
			_RenderPath->RsrcRadianceEnvMap->Apply(*pGPU, _pGlobalAmbientLight->_RadianceEnvMap);

		if (_RenderPath->SampTrilinearCube)
			_RenderPath->SampTrilinearCube->Apply(*pGPU, _TrilinearCubeSampler);
	}
}
//---------------------------------------------------------------------

bool CView::Render()
{
	if (!_RenderPath) return false;

	ZoneScoped;
	ZoneText(_DebugName.c_str(), _DebugName.size());

	// UI-only views can live without scene
	if (_pScene && _pCamera)
	{
		// Synchronize objects from scene to this view
		// TODO: could move SynchronizeRenderables to a job, but now SynchronizeLights is too cheap
		// and this doesn't bring any performance improvement. Need to test again later on a big scene.
		SynchronizeRenderables();
		SynchronizeLights();

		// Check for camera frustum changes
		const bool ViewProjChanged = UpdateCameraFrustum();
		if (ViewProjChanged)
		{
			_Globals.SetMatrix(_RenderPath->ConstViewProjection, GetCamera()->GetViewProjMatrix());
			_Globals.SetVector3(_RenderPath->ConstCameraPosition, GetCamera()->GetPosition());
		}

		// Update visibility flags of spatial tree nodes
		if (ViewProjChanged || _SpatialTreeRebuildVersion != _pScene->GetSpatialTreeRebuildVersion())
		{
			// Invalidate the node visibility cache
			_SpatialTreeNodeVisibility.clear();
			_SpatialTreeRebuildVersion = _pScene->GetSpatialTreeRebuildVersion();
		}
		_pScene->TestSpatialTreeVisibility(_LastViewFrustum, _SpatialTreeNodeVisibility);

		// Update rendering representations of scene objects
		UpdateLights(ViewProjChanged);
		UpdateRenderables(ViewProjChanged);

		// Update queues in parallel as they are independent from each other and don't write to renderables
		// TODO: improve interface to hide a case for no worker? Or stop supporting this situation?
		if (const auto pWorker = _GraphicsMgr->GetJobSystemWorker())
		{
			DEM::Jobs::CJobCounter Counter;
			for (auto& Queue : _RenderQueues)
				pWorker->AddJob(Counter, [&Queue]() { Queue->Update(); });
			// TODO: could do something here to give jobs a chance to finish in the meantime
			pWorker->WaitIdle(Counter);
		}
		else
		{
			for (auto& Queue : _RenderQueues)
				Queue->Update();
		}
	}

	DEM_RENDER_EVENT_SCOPED(GetGPU(), std::wstring(_DebugName.begin(), _DebugName.end()).c_str());

	return _RenderPath->Render(*this);
}
//---------------------------------------------------------------------

// NB: it is recommended to call Present as late as possible after the last
// rendering command. Call it just before rendering the next frame.
// This method results in a no-op for intermediate RTs.
bool CView::Present() const
{
	if (_SwapChainID < 0) FAIL;

	ZoneScoped;

	auto GPU = GetGPU();
	return GPU && GPU->Present(_SwapChainID);
}
//---------------------------------------------------------------------

// FIXME: what if swap chain target is replaced and then window resizing happens?
// Must unsubscribe or resubscribe, dependent on what a new RT is.
bool CView::SetRenderTarget(CStrID ID, Render::PRenderTarget RT)
{
	if (!_RenderPath || !_RenderPath->HasRenderTarget(ID)) FAIL;

	if (RT)
	{
#if DEM_RENDER_DEBUG
		RT->SetDebugName(ID.ToStringView());
#endif

		RTs[ID] = std::move(RT);
	}
	else
	{
		RTs.erase(ID);
	}

	OK;
}
//---------------------------------------------------------------------

Render::CRenderTarget* CView::GetRenderTarget(CStrID ID) const
{
	auto It = RTs.find(ID);
	return (It != RTs.cend()) ? It->second.Get() : nullptr;
}
//---------------------------------------------------------------------

bool CView::SetDepthStencilBuffer(CStrID ID, Render::PDepthStencilBuffer DS)
{
	if (!_RenderPath || !_RenderPath->HasDepthStencilBuffer(ID)) FAIL;

	if (DS)
	{
#if DEM_RENDER_DEBUG
		DS->SetDebugName(ID.ToStringView());
#endif

		DSBuffers[ID] = std::move(DS);
	}
	else
	{
		DSBuffers.erase(ID);
	}

	OK;
}
//---------------------------------------------------------------------

Render::CDepthStencilBuffer* CView::GetDepthStencilBuffer(CStrID ID) const
{
	auto It = DSBuffers.find(ID);
	return (It != DSBuffers.cend()) ? It->second.Get() : nullptr;
}
//---------------------------------------------------------------------

void CView::SetGraphicsScene(CGraphicsScene* pScene)
{
	if (_pScene == pScene) return;

	_pScene = pScene;
	_SpatialTreeRebuildVersion = 0;

	while (!_Renderables.empty())
	{
		auto It = _Renderables.begin();
		It->second.reset();
		_RenderableNodePool.push_back(_Renderables.extract(It));
	}

	while (!_Lights.empty())
	{
		auto It = _Lights.begin();
		It->second.reset();
		_LightNodePool.push_back(_Lights.extract(It));
	}

	_pGlobalAmbientLight = nullptr;
	_GlobalLights.clear();
	_FreeLightGPUIndices.clear();
	_NextUnusedLightGPUIndex = 0;

	for (auto& Queue : _RenderQueues)
		Queue->Clear();
}
//---------------------------------------------------------------------

void CView::SetCamera(CCameraAttribute* pNewCamera)
{
	if (_pCamera != pNewCamera)
	{
		_pCamera = pNewCamera;
		_CameraTfmVersion = 0;
	}
}
//---------------------------------------------------------------------

CGraphicsResourceManager* CView::GetGraphicsManager() const
{
	return _GraphicsMgr;
}
//---------------------------------------------------------------------

Render::CGPUDriver* CView::GetGPU() const
{
	return _GraphicsMgr ? _GraphicsMgr->GetGPU() : nullptr;
}
//---------------------------------------------------------------------

DEM::Sys::COSWindow* CView::GetTargetWindow() const
{
	auto GPU = GetGPU();
	return GPU ? GPU->GetSwapChainWindow(_SwapChainID) : nullptr;
}
//---------------------------------------------------------------------

Render::PDisplayDriver CView::GetTargetDisplay() const
{
	auto GPU = GetGPU();
	return GPU ? GPU->GetSwapChainDisplay(_SwapChainID) : nullptr;
}
//---------------------------------------------------------------------

bool CView::IsFullscreen() const
{
	if (_SwapChainID < 0) FAIL;

	auto GPU = GetGPU();
	return GPU && GPU->IsFullscreen(_SwapChainID);
}
//---------------------------------------------------------------------

// TODO: fullscreen handling here too?
bool CView::OnOSWindowResized(Events::CEventDispatcher* pDispatcher, const Events::CEventBase& Event)
{
	const auto& Ev = static_cast<const Event::OSWindowResized&>(Event);
	auto GPU = GetGPU();

	if (!GPU || Ev.ManualResizingInProgress) FAIL;

	GPU->SetDepthStencilBuffer(nullptr);

	GPU->ResizeSwapChain(_SwapChainID, Ev.NewWidth, Ev.NewHeight);

	//???really need to resize ALL DS buffers? May need some flag declared in a render path or CView::SetDepthStencilAutosized()
	//or add into render path an ability to specify RT/DS size relative to swap chain or some other RT / DS.
	for (auto& [ID, DS] : DSBuffers)
	{
		if (!DS) continue;

		auto Desc = DS->GetDesc();
		Desc.Width = Ev.NewWidth;
		Desc.Height = Ev.NewHeight;

		DS = GPU->CreateDepthStencilBuffer(Desc);
	}

	if (_pCamera)
	{
		_pCamera->SetWidth(Ev.NewWidth);
		_pCamera->SetHeight(Ev.NewHeight);
	}

	OK;
}
//---------------------------------------------------------------------

}
