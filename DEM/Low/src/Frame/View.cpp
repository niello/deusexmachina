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

//!!!DBG TMP!
struct CDummyKey32
{
	using TKey = U32;

	TKey operator()(const Render::IRenderable* /*pRenderable*/) const { return 0; }
};

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

	// Create render queues used by this render path
	_RenderQueues.resize(_RenderPath->_RenderQueues.size());
	for (auto [Type, Index] : _RenderPath->_RenderQueues)
	{
		//???move to factory later? it is not yet well suited for templated classes.
		if (Type == "OpaqueDepthPrePass")
			_RenderQueues[Index] = std::make_unique<CRenderQueue<CDummyKey32, (1 << Render::EEffectType::EffectType_Opaque)>>();
		else if (Type == "AlphaTestDepthPrePass")
			_RenderQueues[Index] = std::make_unique<CRenderQueue<CDummyKey32, (1 << Render::EEffectType::EffectType_AlphaTest)>>();
		else if (Type == "OpaqueMaterial")
			_RenderQueues[Index] = std::make_unique<CRenderQueue<CDummyKey32, (1 << Render::EEffectType::EffectType_Opaque)>>();
		else if (Type == "AlphaTestMaterial")
			_RenderQueues[Index] = std::make_unique<CRenderQueue<CDummyKey32, (1 << Render::EEffectType::EffectType_AlphaTest)>>();
		else if (Type == "AlphaBackToFront")
			_RenderQueues[Index] = std::make_unique<CRenderQueue<CDummyKey32, (1 << Render::EEffectType::EffectType_AlphaBlend)>>();
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
	// apply material overrides when resolveng these pairs into actual shader techniques.
	auto Key = std::make_pair(&Effect, InputSet);
	auto It = _EffectMap.find(Key);
	if (It != _EffectMap.cend()) return It->second;
	const auto NewIndex = static_cast<U32>(_EffectMap.size());
	_EffectMap.emplace(std::move(Key), NewIndex);
	return NewIndex;
}
//---------------------------------------------------------------------

UPTR CView::GetMeshLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const
{
	switch (MeshLODType)
	{
		case LOD_None:
		{
			return 0;
		}
		case LOD_Distance:
		{
			for (UPTR i = 0; i < MeshLODScale.size(); ++i)
				if (SqDistanceToCamera < MeshLODScale[i]) return i;
			return MeshLODScale.size();
		}
		case LOD_ScreenSizeRelative:
		{
			for (UPTR i = 0; i < MeshLODScale.size(); ++i)
				if (ScreenSpaceOccupiedRel > MeshLODScale[i]) return i;
			return MeshLODScale.size();
		}
		case LOD_ScreenSizeAbsolute:
		{
			// FIXME: what render target to use? Not always the first one definitely!
			if (!RTs.begin()->second) return 0;
			const Render::CRenderTargetDesc& RTDesc = RTs.begin()->second->GetDesc();
			const float ScreenSpaceOccupiedAbs = SqDistanceToCamera * RTDesc.Width * RTDesc.Height;
			for (UPTR i = 0; i < MeshLODScale.size(); ++i)
				if (ScreenSpaceOccupiedAbs > MeshLODScale[i]) return i;
			return MeshLODScale.size();
		}
	}

	return 0;
}
//---------------------------------------------------------------------

UPTR CView::GetMaterialLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const
{
	switch (MaterialLODType)
	{
		case LOD_None:
		{
			return 0;
		}
		case LOD_Distance:
		{
			for (UPTR i = 0; i < MaterialLODScale.size(); ++i)
				if (SqDistanceToCamera < MaterialLODScale[i]) return i;
			return MaterialLODScale.size();
		}
		case LOD_ScreenSizeRelative:
		{
			for (UPTR i = 0; i < MaterialLODScale.size(); ++i)
				if (ScreenSpaceOccupiedRel > MaterialLODScale[i]) return i;
			return MaterialLODScale.size();
		}
		case LOD_ScreenSizeAbsolute:
		{
			// FIXME: what render target to use? Not always the first one definitely!
			if (!RTs.begin()->second) return 0;
			const Render::CRenderTargetDesc& RTDesc = RTs.begin()->second->GetDesc();
			const float ScreenSpaceOccupiedAbs = SqDistanceToCamera * RTDesc.Width * RTDesc.Height;
			for (UPTR i = 0; i < MaterialLODScale.size(); ++i)
				if (ScreenSpaceOccupiedAbs > MaterialLODScale[i]) return i;
			return MaterialLODScale.size();
		}
	}

	return 0;
}
//---------------------------------------------------------------------

void CView::Update(float dt)
{
	if (_GraphicsMgr) _GraphicsMgr->Update(dt);
	if (_UIContext) _UIContext->Update(dt);
}
//---------------------------------------------------------------------

void CView::SynchronizeRenderables()
{
	// Synchronize scene objects with their renderable mirrors
	DEM::Algo::SortedUnion(_pScene->GetRenderables(), _Renderables, [](const auto& a, const auto& b) { return a.first < b.first; },
		[this](auto ItSceneObject, auto ItViewObject)
	{
		const CGraphicsScene::CSpatialRecord* pRecord = nullptr;
		Render::IRenderable* pRenderable = nullptr;

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
			pRecord = &ItSceneObject->second;

			auto pAttr = static_cast<CRenderableAttribute*>(pRecord->pAttr);

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

			pRenderable = ItViewObject->second.get();

			//???or test visibility first and delay adding to queues until visible the first time?
			//???contain only visible objects in queues? need to profile what works better!
			for (auto& Queue : _RenderQueues)
				Queue->Add(pRenderable);
		}
		else
		{
			pRecord = &ItSceneObject->second;
			pRenderable = ItViewObject->second.get();

			//???where and when to update world matrix? store it cached in a PRenderable or get from scene object on parallel iteration?

			// TODO:
			// if sorted queue includes distance to camera and bounds changed, mark sorted queue dirty
			// if sorted queue includes material etc which has changed, mark sorted queue dirty
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
		const CGraphicsScene::CSpatialRecord* pRecord = nullptr;
		Render::CLight* pLight = nullptr;

		if (ItSceneObject == _pScene->GetLights().cend())
		{
			// An attribute was removed from a scene, remove its associated GPU light instance
			// NB: erasing a map doesn't affect other iterators, and SortedUnion already cached the next one
			ItViewObject->second.reset();
			_LightNodePool.push_back(_Lights.extract(ItViewObject));

			// TODO: erase this light from hardawre constant buffer and from all objects' light lists!
		}
		else if (ItViewObject == _Lights.cend())
		{
			// A new object in a scene
			const auto UID = ItSceneObject->first;
			pRecord = &ItSceneObject->second;

			auto pAttr = static_cast<CLightAttribute*>(pRecord->pAttr);

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

			pLight = ItViewObject->second.get();

			// ligths without octree node are saved to global, others - to local
			// visibility and intersection with renderables will be calculated only for local lights
			// prioritization will be made for all lights, but global lights have the same intensity at every point in space
			// maybe global lights must bypass prioritization, as they use different constants and algorithms in the shader!
		}
		else
		{
			pRecord = &ItSceneObject->second;
			pLight = ItViewObject->second.get();

			// mark visibility and intersections dirty if bounds version changed
			// NB: for spot light this needs to be updated when tfm changes too, if cone is used for isect. Other lights are transform independent.
			//!!!also remember that intersection with objects is view independent, but in view we can update only visible objects and lights.
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
			//???TODO: need to test for empty bounds and set invisible to save resources on further processing?!

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

			// Calculate LOD. It is based on camera and object bounds so it can be recalculated here only when something of that changes.
			if (pRenderable->IsVisible)
			{
				//const float SqDistanceToCamera = Math::SqDistancePointAABB(_EyePos, Record.BoxCenter, Record.BoxExtent);
				// and/or screen size

				// calculate geometry and material LODs
				// if LODs changed, apply these changes or at least mark related things dirty
				// if LOD disables the object, set it invisible (typically the last LOD): pRenderable->IsVisible = false;
				//???or calc LOD inside UpdateRenderable? and skip some ops if LOD resolves to object hiding?
				//???or calc LOD here and pass to UpdateRenderable? What is better? Pass view pos and viewproj matrix from here or just calculated LOD?
				//???or calc some LOD metric(s) here, e.g. sq distance and screen coverage, and send them to UpdateRenderable?

				//!!!store calculated LOD somewhere!
				//!!!???use one LOD value for both geometry and material, but can actually change something only when required?!
			}
		}

		if (pRenderable->IsVisible)
		{
			//!!!TODO: inside UpdateRenderable:
			// - update tfm from attr
			// - update renderable/light params from attr
			// - update cached GPU structures for dirty parts (tfm, material etc)
			pAttr->UpdateRenderable(*this, *pRenderable);

			// if needed, mark object for updating light intersections

			//???update sorting key for queues?! store it in queue nodes?
		}

		if (WasVisible != pRenderable->IsVisible)
		{
			// if became visible, insert to queues (use sorted insertion? what if order was already broken?), else remove from them
			//???after syncing lists, first sort queues as is, then insert and remove due to visibility?
			//then easier to insert to the end here and sort once after all processing!
			//???or maybe try to insert sorted with possible violation, and then resort as almost sorted and everything will be fixed?
			//but sorted insertion has a cost, it is easier to sort only once!
		}
	}
}
//---------------------------------------------------------------------

void CView::UpdateLights(bool ViewProjChanged)
{
	// Iterate synchronized collections side by side
	auto ItSceneObject = _pScene->GetLights().cbegin();
	auto ItViewObject = _Lights.cbegin();
	for (; ItViewObject != _Lights.cend(); ++ItSceneObject, ++ItViewObject)
	{
		const CGraphicsScene::CSpatialRecord& Record = ItSceneObject->second;
		Render::CLight* pLight = ItViewObject->second.get();
		auto pAttr = static_cast<CLightAttribute*>(Record.pAttr);

		float SqDistanceToCamera = -1.f;
		if (!Record.BoundsVersion)
		{
			// Objects with invalid bounds are always visible. E.g. skybox.
			pLight->IsVisible = true;
			pLight->BoundsVersion = Record.BoundsVersion;
		}
		else if (ViewProjChanged || pLight->BoundsVersion != Record.BoundsVersion)
		{
			//???TODO: need to test for empty bounds and set invisible to save resources on further processing?!

			const bool NoTreeNode = (Record.NodeIndex == NO_SPATIAL_TREE_NODE);
			if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2]) // Check if node has a visible part
			{
				if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2 + 1]) // Check if node has an invisible part
				{
					//!!!FIXME: for lights maybe better is to use spheres! Octree insertion is identical for AABB-Sphere & AABB-AABB, but here spheres are faster!
					pLight->IsVisible = Math::ClipAABB(Record.BoxCenter, Record.BoxExtent, _LastViewFrustum);
				}
				else pLight->IsVisible = true;

				if (pLight->IsVisible) SqDistanceToCamera = Math::SqDistancePointAABB(_EyePos, Record.BoxCenter, Record.BoxExtent);
			}
			else pLight->IsVisible = false;

			pLight->BoundsVersion = Record.BoundsVersion;
		}

		// Light sources that emit no light or that are too far away are considered invisible
		if (pLight->IsVisible && pAttr->DoesEmitAnyEnergy() && SqDistanceToCamera <= pAttr->GetMaxDistanceSquared())
		{
			//!!!TODO: inside UpdateLight:
			// - update tfm from attr
			// - update light params from attr
			// - update cached GPU structures for dirty parts (constant buffer element, IBL textures etc)
			pAttr->UpdateLight(*_GraphicsMgr, *pLight);

			// if needed, mark light for updating object intersections
		}
	}
}
//---------------------------------------------------------------------

void CView::UpdateShaderTechCache()
{
	// Resolve techniques for newly registered effects
	for (UPTR ShaderTechCacheIndex = 0; ShaderTechCacheIndex < _ShaderTechCache.size(); ++ShaderTechCacheIndex)
	{
		auto& TechCache = _ShaderTechCache[ShaderTechCacheIndex];
		const auto CacheSize = TechCache.size();
		if (CacheSize < _EffectMap.size())
		{
			TechCache.resize(_EffectMap.size(), nullptr);
			for (const auto& [EffectAndInputSet, Index] : _EffectMap)
			{
				if (Index >= CacheSize)
				{
					const Render::CEffect* pEffect = EffectAndInputSet.first;
					if (ShaderTechCacheIndex > 0)
					{
						const auto& EffectOverrides = _RenderPath->EffectOverrides[ShaderTechCacheIndex - 1];
						auto OverrideIt = EffectOverrides.find(pEffect->GetType());
						if (OverrideIt != EffectOverrides.cend())
							pEffect = _GraphicsMgr->GetEffect(OverrideIt->second).Get();
					}

					if (pEffect)
						TechCache[Index] = pEffect->GetTechByInputSet(EffectAndInputSet.second);
				}
			}
		}
	}
}
//---------------------------------------------------------------------

bool CView::Render()
{
	if (!_RenderPath || !_pScene || !pCamera) return false;

	// Synchronize objects from scene to this view
	SynchronizeRenderables();
	SynchronizeLights();

	// Check for camera frustum changes
	bool ViewProjChanged = false;
	{
		// View changes are detected easily with a camera node transform version
		const auto CameraTfmVersion = pCamera->GetNode()->GetTransformVersion();
		if (_CameraTfmVersion != CameraTfmVersion)
		{
			ViewProjChanged = true;
			_CameraTfmVersion = CameraTfmVersion;

			const auto& EyePos = pCamera->GetPosition();
			_EyePos = acl::vector_set(EyePos.x, EyePos.y, EyePos.z);
		}

		// Both perspective and orthographic projections are characterized by just few matrix elements, compare only them
		const auto& Proj = pCamera->GetProjMatrix();
		const auto ProjectionParams = acl::vector_set(Proj.m[0][0], Proj.m[1][1], Proj.m[2][2], Proj.m[3][2]);
		if (!acl::vector_all_near_equal(_ProjectionParams, ProjectionParams))
		{
			ViewProjChanged = true;
			_ProjectionParams = ProjectionParams;
		}
	}

	if (ViewProjChanged) _LastViewFrustum = Math::CalcFrustumParams(pCamera->GetViewProjMatrix());

	//!!!FIXME: also need to invalidate cache of changed nodes when one node takes index of another! //???notify SPS->All views for invalidated indices?
	/*if (ViewProjChanged)*/ _SpatialTreeNodeVisibility.clear();
	_pScene->TestSpatialTreeVisibility(_LastViewFrustum, _SpatialTreeNodeVisibility);

	UpdateLights(ViewProjChanged);
	UpdateRenderables(ViewProjChanged);

	//???FIXME: fill queue in visibility update instead of lists sync? contain only visible objects. faster sorting, less iteration and checks, but frequent rebuild.
	//!!!TODO PERF: queues are independent, no write access to renderables is needed, can parallelize!
	for (auto& Queue : _RenderQueues)
		Queue->Update();

	// _pScene->UpdateRenderableLightIntersections() for marked objects and lights
	//???for each light could store a full list of morton codes or at least top level morton codes that are intersecting it!
	//???or is it easier to test against object AABB directly?
	//???does tracking intersections on insert or tracking objects inside node / intersecting the node a good idea?
	//!!!NB: if object and light don't move their intersection doesn't change!

	// TODO:
	// if ViewProjChanged, recalculate distance to camera for all objects, not only for ones with changed bounds.

	// Draw call ordering:
	// 

	UpdateShaderTechCache();

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!DBG TMP!
	VisibilityCache.Clear();
	for (const auto& [UID, Renderable] : _Renderables)
	{
		if (Renderable->IsVisible)
			VisibilityCache.push_back(UID);
	}

	LightCache.Clear();
	// ...

	EnvironmentCache.Clear();
	for (const auto& [UID, Light] : _Lights)
	{
		//!!!
		//if (Light->IsVisible)

		if (auto pAttr = _pScene->GetLights().find(UID)->second.pAttr->As<CIBLAmbientLightAttribute>())
			EnvironmentCache.Add(static_cast<Render::CImageBasedLight*>(Light.get()));

		//	Scene::CNodeAttribute* pAttr = VisibilityCache[i];
		//	if (pAttr->IsA<CLightAttribute>())
		//	{
		//		Render::CLightRecord& Rec = *LightCache.Add();
		//		Rec.pLight = &((CLightAttribute*)pAttr)->GetLight();
		//		Rec.Transform = pAttr->GetNode()->GetWorldMatrix();
		//		Rec.UseCount = 0;
		//		Rec.GPULightIndex = INVALID_INDEX;
		//	}
	}
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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

	// TODO: clear render queues etc!
}
//---------------------------------------------------------------------

void CView::SetCamera(CCameraAttribute* pNewCamera)
{
	if (pCamera != pNewCamera)
	{
		pCamera = pNewCamera;
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

	if (pCamera)
	{
		pCamera->SetWidth(Ev.NewWidth);
		pCamera->SetHeight(Ev.NewHeight);
	}

	OK;
}
//---------------------------------------------------------------------

}
