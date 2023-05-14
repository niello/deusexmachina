#include "View.h"
#include <Frame/CameraAttribute.h>
#include <Frame/Lights/IBLAmbientLightAttribute.h>
#include <Frame/LightAttribute.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/RenderPath.h>
#include <Frame/GraphicsResourceManager.h>
#include <Render/Renderable.h>
#include <Render/RenderTarget.h>
#include <Render/ConstantBuffer.h>
#include <Render/GPUDriver.h>
#include <Render/DisplayDriver.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/Sampler.h>
#include <Render/SamplerDesc.h>
#include <Scene/SceneNode.h>
#include <Debug/DebugDraw.h>
#include <Data/Algorithms.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
#include <Math/CameraMath.h>
#include <System/OSWindow.h>
#include <System/SystemEvents.h>
#include <acl/math/vector4_32.h>

namespace Frame
{

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

	return true;
}
//---------------------------------------------------------------------

//!!!IRenderable children may now serve as parts of the rendering cache / render nodes!
Render::IRenderable* CView::GetRenderObject(const CRenderableAttribute& Attr)
{
	auto It = _RenderObjects.find(&Attr);
	if (It == _RenderObjects.cend())
	{
		// Different views on the same GPU will create different renderables,
		// which is not necessary. Render object cache might be stored in a
		// graphics manager, but it seems wrong for now. Anyway, GPU resources
		// are shared between these objects and views.
		auto Renderable = Attr.CreateRenderable(*_GraphicsMgr);
		if (!Renderable) return nullptr;
		It = _RenderObjects.emplace(&Attr, std::move(Renderable)).first;
	}

	return It->second.get();
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
		const CGraphicsScene::CObjectRecord* pRecord = nullptr;
		Render::IRenderable* pRenderable = nullptr;

		if (ItSceneObject == _pScene->GetRenderables().cend())
		{
			// An object was removed from a scene, remove its renderable
			// NB: erasing a map doesn't affect other iterators, and SortedUnion already cached the next one
			ItViewObject->second.reset();
			_RenderableNodePool.push_back(_Renderables.extract(ItViewObject));

			// TODO:
			// erase from sorted queues
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
				ItViewObject = _Renderables.emplace_hint(_Renderables.cend(), UID, pAttr->CreateRenderable(*_GraphicsMgr));
			}
			else
			{
				auto& Node = _RenderableNodePool.back();
				Node.key() = UID;
				Node.mapped() = pAttr->CreateRenderable(*_GraphicsMgr);
				ItViewObject = _Renderables.insert(_Renderables.cend(), std::move(Node));
				_RenderableNodePool.pop_back();
			}

			pRenderable = ItViewObject->second.get();

			// TODO:
			// add to sorted queues (or test visibility first and delay adding to queues until visible the first time?)
			//!!!an object may be added not to all queues. E.g. alpha objects don't participate in Z prepass and can ignore FrontToBack queue!
			//???what if combining queues in some phases? E.g. make OpaqueMaterial, AlphaBackToFront, sort there only by that factor, and in a phase
			//just render one queue and then another!? Could make sorting faster than for the whole pool of objects.
		}
		else
		{
			pRecord = &ItSceneObject->second;
			pRenderable = ItViewObject->second.get();

			// TODO:
			// if sorted queue includes distance to camera and bounds changed, mark sorted queue dirty
			// if sorted queue includes material etc which has changed, mark sorted queue dirty
		}
	});
}
//---------------------------------------------------------------------

void CView::SynchronizeLights()
{
	// ligths without octree node are saved to global, others - to local
	// visibility and intersection with renderables will be calculated only for local lights
	// prioritization will be made for all lights, but global lights have the same intensity at every point in space
	// maybe global lights must bypass prioritization, as they use different constants and algorithms in the shader!

	// Synchronize scene lights with their GPU mirrors
	//DEM::Algo::SortedUnion(_pScene->GetLights(), _Lights, [](const auto& a, const auto& b) { return a.first < b.first; },
	//	[this](auto ItSceneObject, auto ItViewObject)
	//{
	//	//...
	//});

	//CArray<Scene::CNodeAttribute*> VisibilityCache2;

	//_pScene->QueryObjectsInsideFrustum(pCamera->GetViewProjMatrix(), VisibilityCache2);

	//for (UPTR i = 0; i < VisibilityCache2.GetCount(); /**/)
	//{
	//	Scene::CNodeAttribute* pAttr = VisibilityCache2[i];
	//	if (pAttr->IsA<CLightAttribute>())
	//	{
	//		Render::CLightRecord& Rec = *LightCache.Add();
	//		Rec.pLight = &((CLightAttribute*)pAttr)->GetLight();
	//		Rec.Transform = pAttr->GetNode()->GetWorldMatrix();
	//		Rec.UseCount = 0;
	//		Rec.GPULightIndex = INVALID_INDEX;

	//		//VisibilityCache.RemoveAt(i);
	//	}
	//	else if (pAttr->IsA<CIBLAmbientLightAttribute>())
	//	{
	//		EnvironmentCache.Add(pAttr->As<CIBLAmbientLightAttribute>());
	//		//VisibilityCache.RemoveAt(i);
	//	}
	//	/*else*/ ++i;
	//}

	// CIBLAmbientLightAttribute:
	//if (!pAttrTyped->ValidateGPUResources(*_GraphicsMgr)) FAIL;
	//IrradianceMap = ResMgr.GetTexture(IrradianceMapUID, Render::Access_GPU_Read);
	//RadianceEnvMap = ResMgr.GetTexture(RadianceEnvMapUID, Render::Access_GPU_Read);
}
//---------------------------------------------------------------------

void CView::UpdateObjectVisibility(bool ViewProjChanged)
{
	//!!!???TODO: could keep Frustum from the prev frame if !ViewProjChanged!?
	const auto Frustum = Math::CalcFrustumParams(pCamera->GetViewProjMatrix());

	//!!!FIXME: also need to invalidate cache of changed nodes when one node takes index of another! //???notify SPS->All views for invalidated indices?
	/*if (ViewProjChanged)*/ _SpatialTreeNodeVisibility.clear();
	_pScene->TestSpatialTreeVisibility(Frustum, _SpatialTreeNodeVisibility);

	// Update visibility of renderables. Iterate synchronized collections side by side.
	auto ItSceneObject = _pScene->GetRenderables().cbegin();
	auto ItViewObject = _Renderables.cbegin();
	for (; ItViewObject != _Renderables.cend(); ++ItSceneObject, ++ItViewObject)
	{
		const CGraphicsScene::CObjectRecord& Record = ItSceneObject->second;
		Render::IRenderable* pRenderable = ItViewObject->second.get();
		if (ViewProjChanged || pRenderable->BoundsVersion != Record.BoundsVersion)
		{
			if (Record.BoundsValid)
			{
				const bool NoTreeNode = (Record.NodeIndex == NO_SPATIAL_TREE_NODE);
				if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2]) // Check if node has a visible part
				{
					if (NoTreeNode || _SpatialTreeNodeVisibility[Record.NodeIndex * 2 + 1]) // Check if node has an invisible part
					{
						pRenderable->IsVisible = Math::ClipAABB(Record.BoxCenter, Record.BoxExtent, Frustum);
					}
					else pRenderable->IsVisible = true;
				}
				else pRenderable->IsVisible = false;
			}
			else pRenderable->IsVisible = true; // Objects with invalid bounds are always visible. E.g. skybox.

			pRenderable->BoundsVersion = Record.BoundsVersion;
		}
	}
}
//---------------------------------------------------------------------

bool CView::Render()
{
	if (!_RenderPath) FAIL;

	VisibilityCache.Clear();
	LightCache.Clear();
	EnvironmentCache.Clear();
	VisibilityCacheDirty = true;

	///////// NEW RENDER /////////
	if (_pScene && pCamera)
	{
		bool ViewProjChanged = false;
		{
			// View changes are detected easily with a camera node transform version
			const auto CameraTfmVersion = pCamera->GetNode()->GetTransformVersion();
			if (_CameraTfmVersion != CameraTfmVersion)
			{
				ViewProjChanged = true;
				_CameraTfmVersion = CameraTfmVersion;
			}

			// Both perspective and orthographic projections are characterized by just few matrix elements, compare only them
			const auto& Proj = pCamera->GetProjMatrix();
			const vector4 ProjectionParams(Proj.m[0][0], Proj.m[1][1], Proj.m[2][2], Proj.m[3][2]);
			if (_ProjectionParams != ProjectionParams)
			{
				ViewProjChanged = true;
				_ProjectionParams = ProjectionParams;
			}
		}

		SynchronizeRenderables();
		UpdateObjectVisibility(ViewProjChanged);

		// TODO:
		// if ViewProjChanged, mark all queues which use distance to camera dirty
		// update dirty sorted queues with insertion sort, O(n) for almost sorted arrays. Fallback to qsort for major reorderings or first init.
		//!!!from huge camera changes can mark a flag 'MajorChanges' camera-dependent (FrontToBack etc) queue, and use qsort instead of almost-sorted.

		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//!!!DBG TMP!
		//VisibilityCacheDirty = false;
		VisibilityCache.Clear();
		for (const auto& [UID, Renderable] : _Renderables)
		{
			if (Renderable->IsVisible)
				VisibilityCache.push_back(_pScene->GetRenderables().find(UID)->second.pAttr);
		}
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}
	//////////////////////////////

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
	_RenderObjects.clear();
	VisibilityCacheDirty = true;

	///////// NEW RENDER /////////
	while (!_Renderables.empty())
	{
		auto It = _Renderables.begin();
		It->second.reset();
		_RenderableNodePool.push_back(_Renderables.extract(It));
	}
}
//---------------------------------------------------------------------

bool CView::SetCamera(CCameraAttribute* pNewCamera)
{
	if (pCamera == pNewCamera) OK;
	if (!pNewCamera)
	{
		pCamera = nullptr;
		OK;
	}

	Scene::CSceneNode* pNode = pNewCamera->GetNode();
	if (!pNode) FAIL;

	pCamera = pNewCamera;
	VisibilityCacheDirty = true;
	OK;
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
