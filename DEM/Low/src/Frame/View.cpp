#include "View.h"
#include <Frame/CameraAttribute.h>
#include <Frame/AmbientLightAttribute.h>
#include <Frame/LightAttribute.h>
#include <Frame/RenderableAttribute.h>
#include <Frame/RenderPath.h>
#include <Frame/GraphicsResourceManager.h>
#include <Scene/SPS.h>
#include <Render/Renderable.h>
#include <Render/RenderTarget.h>
#include <Render/ConstantBuffer.h>
#include <Render/GPUDriver.h>
#include <Render/DisplayDriver.h>
#include <Render/DepthStencilBuffer.h>
#include <Render/Sampler.h>
#include <Render/SamplerDesc.h>
#include <Debug/DebugDraw.h>
#include <Data/Algorithms.h>
#include <UI/UIContext.h>
#include <UI/UIServer.h>
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

bool CView::PrecreateRenderObjects(Scene::CSceneNode& RootNode)
{
	return RootNode.Visit([this](Scene::CSceneNode& Node)
	{
		for (UPTR i = 0; i < Node.GetAttributeCount(); ++i)
		{
			Scene::CNodeAttribute& Attr = *Node.GetAttribute(i);

			if (auto pAttrTyped = Attr.As<Frame::CRenderableAttribute>())
			{
				if (!GetRenderObject(*pAttrTyped)) FAIL;
			}
			else if (auto pAttrTyped = Attr.As<Frame::CAmbientLightAttribute>())
			{
				//???as renderable? or to separate cache? Use as a light type? Global IBL is much like
				//directional light, and local is much like omni with ith influence volume!
				if (!pAttrTyped->ValidateGPUResources(*_GraphicsMgr)) FAIL;
			}
		}
		OK;
	});
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

void CView::UpdateVisibilityCache()
{
	if (!VisibilityCacheDirty) return;

	if (_pSPS && pCamera)
	{
		_pSPS->QueryObjectsInsideFrustum(pCamera->GetViewProjMatrix(), VisibilityCache);

		for (UPTR i = 0; i < VisibilityCache.GetCount(); /**/)
		{
			Scene::CNodeAttribute* pAttr = VisibilityCache[i];
			if (pAttr->IsA<CLightAttribute>())
			{
				Render::CLightRecord& Rec = *LightCache.Add();
				Rec.pLight = &((CLightAttribute*)pAttr)->GetLight();
				Rec.Transform = pAttr->GetNode()->GetWorldMatrix();
				Rec.UseCount = 0;
				Rec.GPULightIndex = INVALID_INDEX;

				VisibilityCache.RemoveAt(i);
			}
			else if (pAttr->IsA<CAmbientLightAttribute>())
			{
				EnvironmentCache.Add(pAttr->As<CAmbientLightAttribute>());
				VisibilityCache.RemoveAt(i);
			}
			else ++i;
		}
	}
	else
	{
		VisibilityCache.Clear();
		LightCache.Clear();
		EnvironmentCache.Clear();
	}

	VisibilityCacheDirty = false;
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

bool CView::Render()
{
	if (!_RenderPath) FAIL;

	VisibilityCache.Clear();
	LightCache.Clear();
	EnvironmentCache.Clear();
	VisibilityCacheDirty = true;

	///////// NEW RENDER /////////
	if (_pSPS && pCamera)
	{
		const auto& ViewProj = pCamera->GetViewProjMatrix();

		//???view - can compare tfm version, but how to check projection change? compare final matrices? or source values?
		//!!!to compare projection without view, can check only certain matrix values, because most of them are 0.f for any projection, 5 floats are not.
		const bool ViewProjChanged = true; // TODO: IMPLEMENT!!!

		//!!!TODO: could build planes from GetViewProjMatrix() here and pass to TestSpatialTreeVisibility and also use here for object visibility tests!
		//Make a struct for ViewFrustumPlanes and inside make 2 impls, for SSE and for AVX (ymm, 6 planes at once). forceinline test(struct, box) with 2 impls inside.

		//!!!???TODO: could even keep values from the prev frame if viewproj didn't change!?

		// Extract Left, Right, Bottom & Top frustum planes using Gribb-Hartmann method.
		// Left = W + X, Right = W - X, Bottom = W + Y, Top = W - Y. Normals look inside the frustum, i.e. positive halfspace means 'inside'.
		// LRBT_w is a -d, i.e. -dot(PlaneNormal, PlaneOrigin). +w instead of -d allows us using a single fma instead of separate mul & sub in a box test.
		// See https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
		// See https://fgiesen.wordpress.com/2012/08/31/frustum-planes-from-the-projection-matrix/
		const auto LRBT_Nx = acl::vector_add(acl::vector_set(ViewProj.m[0][3]), acl::vector_set(ViewProj.m[0][0], -ViewProj.m[0][0], ViewProj.m[0][1], -ViewProj.m[0][1]));
		const auto LRBT_Ny = acl::vector_add(acl::vector_set(ViewProj.m[1][3]), acl::vector_set(ViewProj.m[1][0], -ViewProj.m[1][0], ViewProj.m[1][1], -ViewProj.m[1][1]));
		const auto LRBT_Nz = acl::vector_add(acl::vector_set(ViewProj.m[2][3]), acl::vector_set(ViewProj.m[2][0], -ViewProj.m[2][0], ViewProj.m[2][1], -ViewProj.m[2][1]));
		const auto LRBT_w = acl::vector_add(acl::vector_set(ViewProj.m[3][3]), acl::vector_set(ViewProj.m[3][0], -ViewProj.m[3][0], ViewProj.m[3][1], -ViewProj.m[3][1]));

		const auto LRBT_Abs_Nx = acl::vector_abs(LRBT_Nx);
		const auto LRBT_Abs_Ny = acl::vector_abs(LRBT_Ny);
		const auto LRBT_Abs_Nz = acl::vector_abs(LRBT_Nz);

		// Near and far planes are parallel, which enables us to save some work. Near & far planes are +d (-w) along the look
		// axis, so NearPlane is negated once and FarPlane is negated twice (once to make it -w, once to invert the axis).
		// We use D3D style projection matrix, near Z limit is 0 instead of OpenGL's -W.
		// TODO: check if it is really faster than making NF_Nx etc. At least less registers used? Also could use AVX to handle all 6 planes at once.
		const auto NearAxis = acl::vector_set(ViewProj.m[0][2], ViewProj.m[1][2], ViewProj.m[2][2], 0.f);
		const auto FarAxis = acl::vector_sub(acl::vector_set(ViewProj.m[0][3], ViewProj.m[1][3], ViewProj.m[2][3], 0.f), NearAxis);
		const float InvNearLen = acl::sqrt_reciprocal(acl::vector_length_squared3(NearAxis));
		const auto LookAxis = acl::vector_mul(NearAxis, InvNearLen);
		const float NearPlane = -ViewProj.m[3][2] * InvNearLen;
		const float FarPlane = (ViewProj.m[3][3] - ViewProj.m[3][2]) * acl::sqrt_reciprocal(acl::vector_length_squared3(FarAxis));

		//!!!FIXME: also need to invalidate cache of changed nodes when one node takes index of another! //???notify SPS->All views for invalidated indices?
		/*if (ViewProjChanged)*/ _TreeNodeVisibility.clear();
		_pSPS->TestSpatialTreeVisibility(ViewProj, _TreeNodeVisibility);

		// Synchronize scene objects with their renderable mirrors
		DEM::Algo::SortedUnion(_pSPS->GetObjects(), _Renderables, [](const auto& a, const auto& b) { return a.first < b.first; },
			// FIXME: this will be shorter when frustum will live in a struct. But check inlining of the lambda!
			[this, ViewProjChanged, LRBT_Nx, LRBT_Ny, LRBT_Nz, LRBT_w, LRBT_Abs_Nx, LRBT_Abs_Ny, LRBT_Abs_Nz, LookAxis, NearPlane, FarPlane](auto ItSceneObject, auto ItRenderObject)
		{
			const Scene::CSPSRecord* pRecord = nullptr;
			Render::IRenderable* pRenderable = nullptr;
			bool TestVisibility = false;

			if (ItSceneObject == _pSPS->GetObjects().cend())
			{
				// An object was removed from a scene, remove its renderable
				// NB: erasing a map doesn't affect other iterators, and SortedUnion already cached the next one
				ItRenderObject->second.reset();
				_RenderableNodePool.push_back(_Renderables.extract(ItRenderObject));

				// erase from sorted queues
			}
			else if (ItRenderObject == _Renderables.cend())
			{
				// A new object in a scene
				const auto UID = ItSceneObject->first;
				pRecord = ItSceneObject->second;

				//!!!FIXME: need to guarantee this! Split GetObjects by type on insertion? Lights etc don't need renderables and sync!
				n_assert_dbg(pRecord->pUserData->As<Frame::CRenderableAttribute>());
				auto pAttr = static_cast<const Frame::CRenderableAttribute*>(pRecord->pUserData);

				// UIDs always grow (unless overflowed), and therefore adding to the end is always the right hint which gives us O(1) insertion
				//???!!!could compact UIDs when close to overflow! Do in SPS and broadcast changes OldUID->NewUID to all views. With guarantee of
				//doing this in order, we can keep an iterator and avoid logarithmic searches for each change!
				if (_RenderableNodePool.empty())
				{
					ItRenderObject = _Renderables.emplace_hint(ItRenderObject, UID, pAttr->CreateRenderable(*_GraphicsMgr));
				}
				else
				{
					ItRenderObject = _Renderables.insert(ItRenderObject, std::move(_RenderableNodePool.back()));
					_RenderableNodePool.pop_back();
					ItRenderObject->second = pAttr->CreateRenderable(*_GraphicsMgr);
				}

				pRenderable = ItRenderObject->second.get();
				TestVisibility = true;

				// add to sorted queues (or test visibility first and delay adding to queues until visible the first time?)
				//!!!an object may be added not to all queues. E.g. alpha objects don't participate in Z prepass and can ignore FrontToBack queue!
				//???what if combining queues in some phases? E.g. make OpaqueMaterial, AlphaBackToFront, sort there only by that factor, and in a phase
				//just render one queue and then another!? Could make sorting faster than for the whole pool of objects.
			}
			else
			{
				pRecord = ItSceneObject->second;
				pRenderable = ItRenderObject->second.get();

				TestVisibility = (ViewProjChanged || pRenderable->BoundsVersion != pRecord->BoundsVersion);

				// if sorted queue includes distance to camera and bounds changed, mark sorted queue dirty
				// if sorted queue includes material etc which has changed, mark sorted queue dirty
			}

			if (TestVisibility)
			{
				// Update visibility
				//!!!???TODO: to inline function?!
				if (_TreeNodeVisibility[pRecord->NodeIndex * 2]) // Check if node has a visible part
				{
					if (_TreeNodeVisibility[pRecord->NodeIndex * 2 + 1]) // Check if node has an invisible part
					{
						const auto BoxCenter = pRecord->BoxCenter;
						const auto BoxExtent = pRecord->BoxExtent;

						//???check AABB or OBB? for OBB, can make WVP matrix and do the same for it? Probably more expensive, can't reuse planes!
						// OBB:  float r = b.e[0]*Abs(Dot(p.n, b.u[0])) + b.e[1]*Abs(Dot(p.n, b.u[1])) + b.e[2]*Abs(Dot(p.n, b.u[2]));
						// AABB: float r = b.e[0]*Abs(p.n[0])           + b.e[1]*Abs(p.n[1])           + b.e[2]*Abs(p.n[2]);

						// Distance of box center from plane (s): (Cx * Nx) + (Cy * Ny) + (Cz * Nz) - d, where "- d" is "+ w"
						auto CenterDistance = acl::vector_mul_add(acl::vector_mix_xxxx(BoxCenter), LRBT_Nx, LRBT_w);
						CenterDistance = acl::vector_mul_add(acl::vector_mix_yyyy(BoxCenter), LRBT_Ny, CenterDistance);
						CenterDistance = acl::vector_mul_add(acl::vector_mix_zzzz(BoxCenter), LRBT_Nz, CenterDistance);

						// Projection radius of the most inside vertex: Ex * abs(Nx) + Ey * abs(Ny) + Ez * abs(Nz)
						auto ProjectedExtent = acl::vector_mul(acl::vector_mix_xxxx(BoxExtent), LRBT_Abs_Nx);
						ProjectedExtent = acl::vector_mul_add(acl::vector_mix_yyyy(BoxExtent), LRBT_Abs_Ny, ProjectedExtent);
						ProjectedExtent = acl::vector_mul_add(acl::vector_mix_zzzz(BoxExtent), LRBT_Abs_Nz, ProjectedExtent);

						//if (acl::vector_any_greater_equal(CenterDistance, ProjectedExtent))
						//	return false;

						//return true;

						//!!!add near-far check!

						// pRenderable->IsVisible = make bounds test
					}
					else pRenderable->IsVisible = true;
				}
				else pRenderable->IsVisible = false;

				pRenderable->BoundsVersion = pRecord->BoundsVersion;
			}

			// if ViewProjChanged, mark all queues which use distance to camera dirty
			// update dirty sorted queues with insertion sort, O(n) for almost sorted arrays. Fallback to qsort for major reorderings or first init.
			//!!!from huge camera changes can mark a flag 'MajorChanges' camera-dependent (FrontToBack etc) queue, and use qsort instead of almost-sorted.
		});
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

void CView::SetScene(Scene::CSPS* pSPS)
{
	if (_pSPS == pSPS) return;

	_pSPS = pSPS;
	_RenderObjects.clear();
	VisibilityCacheDirty = true;
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
