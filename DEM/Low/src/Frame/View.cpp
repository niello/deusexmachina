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
#include <acl/math/vector4_32.h>

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
	Globals = Render::CShaderParamStorage(GlobalParams, *GraphicsMgr.GetGPU(), true);
	for (const auto& Const : GlobalParams.GetConstants())
		Globals.CreatePermanentConstantBuffer(Const.GetConstantBufferIndex(), Render::Access_CPU_Write | Render::Access_GPU_Read);

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
	TrilinearCubeSampler = GraphicsMgr.GetGPU()->CreateSampler(SamplerDesc);

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
		if (!Renderer || !Renderer->Init(*RendererSettings.SettingsDesc)) continue;

		for (const auto pRTTI : RendererSettings.RenderableTypes)
			_RenderersByRenderableType.emplace(pRTTI, static_cast<U8>(_Renderers.size()));

		_Renderers.push_back(std::move(Renderer));
	}

	// Create render queues used by this render path
	_RenderQueues.resize(_RenderPath->_RenderQueues.size());
	for (auto [Type, Index] : _RenderPath->_RenderQueues)
	{
		//???TODO: move to factory later? it is not yet well suited for templated classes.
		//!!!TODO: for x64 use 64-bit keys with more counter space for meshes and materials!
		//???need separated opaque and atest in a color phase?! atest is in render state, sorting by tech already happens.
		if (Type == "OpaqueDepthPrePass")
			_RenderQueues[Index] = std::make_unique<Render::CRenderQueue<CMaterialKey32>>(ENUM_MASK(Render::EEffectType::EffectType_Opaque));
		else if (Type == "AlphaTestDepthPrePass")
			// FIXME: also would benefit from FtB sorting here, see CAlphaTestDepthPrePass64, but can't use on 32-bit!
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

		const auto& EyePos = _pCamera->GetPosition();
		_EyePos = acl::vector_set(EyePos.x, EyePos.y, EyePos.z);
	}

	// Both perspective and orthographic projections are characterized by just few matrix elements, compare only them
	const auto& Proj = _pCamera->GetProjMatrix();
	const auto ProjectionParams = acl::vector_set(Proj.m[0][0], Proj.m[1][1], Proj.m[2][2], Proj.m[3][2]);
	if (!acl::vector_all_near_equal(_ProjectionParams, ProjectionParams))
	{
		ViewProjChanged = true;
		_ProjectionParams = ProjectionParams;
		_ScreenMultiple = std::max(0.5f * Proj.m[0][0], 0.5f * Proj.m[1][1]);
	}

	if (ViewProjChanged)
		_LastViewFrustum = Math::CalcFrustumParams(_pCamera->GetViewProjMatrix());

	return ViewProjChanged;
}
//---------------------------------------------------------------------

void CView::SynchronizeRenderables()
{
	// Synchronize scene objects with their renderable mirrors
	DEM::Algo::SortedUnion(_pScene->GetRenderables(), _Renderables, [](const auto& a, const auto& b) { return a.first < b.first; },
		[this](auto ItSceneObject, auto ItViewObject)
	{
		if (ItSceneObject == _pScene->GetRenderables().cend())
		{
			// An object was removed from a scene, remove its renderable from queues and from a sync list
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
	// Synchronize scene lights with their GPU mirrors
	DEM::Algo::SortedUnion(_pScene->GetLights(), _Lights, [](const auto& a, const auto& b) { return a.first < b.first; },
		[this](auto ItSceneObject, auto ItViewObject)
	{
		if (ItSceneObject == _pScene->GetLights().cend())
		{
			// TODO: erase this light from hardware constant buffer and from all objects' light lists!

			// An attribute was removed from a scene, remove its associated GPU light instance
			// NB: erasing a map doesn't affect other iterators, and SortedUnion already cached the next one
			ItViewObject->second.reset();
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
			if (_LightNodePool.empty())
			{
				ItViewObject = _Lights.emplace_hint(_Lights.cend(), UID, pAttr->CreateLight());
			}
			else
			{
				auto& Node = _LightNodePool.back();
				Node.key() = UID;
				Node.mapped() = pAttr->CreateLight();
				ItViewObject = _Lights.insert(_Lights.cend(), std::move(Node));
				_LightNodePool.pop_back();
			}
		}
	});
}
//---------------------------------------------------------------------

void CView::UpdateRenderables(bool ViewProjChanged)
{
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
					pRenderable->IsVisible = Math::ClipAABB(Record.BoxCenter, Record.BoxExtent, _LastViewFrustum);
				}
				else pRenderable->IsVisible = true;
			}
			else pRenderable->IsVisible = false;

			pRenderable->BoundsVersion = Record.BoundsVersion;

			// Calculate LOD prerequisites. They depend only on camera and object bounds.
			if (pRenderable->IsVisible)
			{
				pRenderable->DistanceToCamera = std::sqrtf(Math::SqDistancePointAABB(_EyePos, Record.BoxCenter, Record.BoxExtent));
				pRenderable->RelScreenRadius = _ScreenMultiple * acl::vector_get_w(Record.Sphere) / std::max(acl::vector_distance3(_EyePos, Record.BoxCenter), 1.0f);
			}
		}

		// Update a renderable from its scene attribute. NB: IsVisible may be set to false inside.
		if (pRenderable->IsVisible)
			pAttr->UpdateRenderable(*this, *pRenderable);

		//!!!DBG TMP! Or is it the preferred way to do this? Or handle in UpdateRenderable? But every renderable stores tfm identically. Or no?
		//???could store directly in per-instance data? or it depends on a renderer too strongly?
		if (pRenderable->IsVisible)
			pRenderable->Transform = pAttr->GetNode()->GetWorldMatrix();

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

		// Setup object-light intersection tracking
		// TODO: for pairs of static object and static light might use lightmapping or other optimizations. Exploit physics lib's collision?
		// TODO: _IsForwardLighting OR shadows / alpha etc, what needs intersections in deferred lighting
		// TODO: don't search for intersection for unlit materials if not needed for shadow
		const bool TrackObjectLightIntersections = pRenderable->IsVisible && Record.BoundsVersion && _RenderPath->_IsForwardLighting;
		if (TrackObjectLightIntersections != pRenderable->TrackObjectLightIntersections)
		{
			_pScene->TrackObjectLightIntersections(*pAttr, TrackObjectLightIntersections);
			pRenderable->TrackObjectLightIntersections = TrackObjectLightIntersections;
		}
		if (TrackObjectLightIntersections)
		{
			_pScene->UpdateObjectLightIntersections(*pAttr);

			if (Record.ObjectLightIntersectionsVersion != pRenderable->ObjectLightIntersectionsVersion)
				pAttr->UpdateLightList(*this, *pRenderable, Record.pObjectLightIntersections);
		}
	}
}
//---------------------------------------------------------------------

bool CView::UpdateLights(bool ViewProjChanged)
{
	bool VisibleLightsChanged = false;

	// Iterate synchronized collections side by side
	auto ItSceneObject = _pScene->GetLights().cbegin();
	auto ItViewObject = _Lights.cbegin();
	for (; ItViewObject != _Lights.cend(); ++ItSceneObject, ++ItViewObject)
	{
		const CGraphicsScene::CSpatialRecord& Record = ItSceneObject->second;
		Render::CLight* pLight = ItViewObject->second.get();
		auto pAttr = static_cast<CLightAttribute*>(Record.pAttr);
		const bool WasVisible = pLight->IsVisible;

		float DistanceToCamera = 0.f;
		if (!Record.BoundsVersion)
		{
			// Lights with invalid bounds are global and therefore always visible
			pLight->IsVisible = true;
			pLight->BoundsVersion = Record.BoundsVersion;
		}
		else if (ViewProjChanged || pLight->BoundsVersion != Record.BoundsVersion)
		{
			//???TODO: need to test for zero bounds and set invisible to save resources on further processing?!

			const bool NoTreeNode = (Record.NodeIndex == NO_SPATIAL_TREE_NODE);
			if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2]) // Check if node has a visible part
			{
				if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2 + 1]) // Check if node has an invisible part
				{
					pLight->IsVisible = Math::ClipSphere(Record.Sphere, _LastViewFrustum);
				}
				else pLight->IsVisible = true;

				if (pLight->IsVisible) DistanceToCamera = Math::DistancePointSphere(_EyePos, Record.Sphere);
			}
			else pLight->IsVisible = false;

			pLight->BoundsVersion = Record.BoundsVersion;
		}

		if (pLight->IsVisible)
		{
			// Light sources that are too far away are considered invisible. UpdateLight can also make a light invisible.
			// TODO: if store distance to camera in the light itself, can check this inside UpdateLight, as with LOD for renderables.
			//???also use screen size, everything as for renderables?! can be useful for culling lights that affect few pixels!
			if (DistanceToCamera <= pAttr->GetMaxDistance())
				pAttr->UpdateLight(*_GraphicsMgr, *pLight);
			else
				pLight->IsVisible = false;
		}

		VisibleLightsChanged |= (WasVisible != pLight->IsVisible);

		// Setup object-light intersection tracking
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

	return VisibleLightsChanged;
}
//---------------------------------------------------------------------

bool CView::Render()
{
	if (!_RenderPath || !_pScene || !_pCamera) return false;

	// Synchronize objects from scene to this view
	SynchronizeRenderables();
	SynchronizeLights();

	// Check for camera frustum changes
	const bool ViewProjChanged = UpdateCameraFrustum();

	// Update visibility flags of spatial tree nodes
	if (ViewProjChanged || _SpatialTreeRebuildVersion != _pScene->GetSpatialTreeRebuildVersion())
	{
		// Invalidate the node visibility cache
		_SpatialTreeNodeVisibility.clear();
		_SpatialTreeRebuildVersion = _pScene->GetSpatialTreeRebuildVersion();
	}
	_pScene->TestSpatialTreeVisibility(_LastViewFrustum, _SpatialTreeNodeVisibility);

	// Update rendering representations of scene lights
	if (UpdateLights(ViewProjChanged))
	{
		// TODO: fill global light list and other globals here or in CRenderPath::Render! Dir lights - to separate GPU structures, choose N most intense/high-priority.
		// use CLight::FillGPUStructure(DataRef) to avoid RTTI casts, allow each light to fill its structure, incl. type enum.
		//???send all or only visible lights to GPU?
		//!!!split into global lights (IBL global, directional) which will be prioritized to limited slots, and local lights which will be stored in a GPU buffer
		//???call UpdateLightList with lights already resolved into Render::CLight?
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//!!!DBG TMP!
		EnvironmentCache.Clear();
		for (const auto& [UID, Light] : _Lights)
		{
			if (Light->IsVisible)
			{
				// if (Light->BoundsVersion) ...local... else ...global...

				if (_pScene->GetLights().find(UID)->second.pAttr->IsA<CIBLAmbientLightAttribute>())
				{
					EnvironmentCache.Add(static_cast<Render::CImageBasedLight*>(Light.get()));
				}
				else
				{
					//Render::CLightRecord& Rec = *LightCache.Add();
					//Rec.pLight = &((CLightAttribute*)pAttr)->GetLight();
					//Rec.Transform = pAttr->GetNode()->GetWorldMatrix();
					//Rec.UseCount = 0;
					//Rec.GPULightIndex = INVALID_INDEX;
				}
			}
		}
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	// Update rendering representations of scene objects
	UpdateRenderables(ViewProjChanged);

	//!!!TODO PERF: queues are independent, no write access to renderables is needed, can parallelize!
	for (auto& Queue : _RenderQueues)
		Queue->Update();

	return _RenderPath->Render(*this);
}
//---------------------------------------------------------------------

// NB: it is recommended to call Present as late as possible after the last
// rendering command. Call it just before rendering the next frame.
// This method results in a no-op for intermediate RTs.
bool CView::Present() const
{
	if (_SwapChainID < 0) FAIL;

	auto GPU = GetGPU();
	return GPU && GPU->Present(_SwapChainID);
}
//---------------------------------------------------------------------

// FIXME: what if swap chain target is replaced and then window resizing happens?
// Must unsubscribe or resubscribe, dependent on what a new RT is.
bool CView::SetRenderTarget(CStrID ID, Render::PRenderTarget RT)
{
	if (!_RenderPath || !_RenderPath->HasRenderTarget(ID)) FAIL;

	if (RT) RTs[ID] = RT;
	else RTs.erase(ID);
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

	if (DS) DSBuffers[ID] = DS;
	else DSBuffers.erase(ID);
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
