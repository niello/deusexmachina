#pragma once
#ifndef __DEM_L1_FRAME_VIEW_H__
#define __DEM_L1_FRAME_VIEW_H__

#include <Render/RenderFwd.h>
#include <Render/RenderNode.h>
#include <Render/EffectConstSetValues.h>
#include <Data/FixedArray.h>
#include <Data/Array.h>

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
typedef Ptr<class CRenderPath> PRenderPath;

enum ELODType
{
	LOD_None,				// No LOD, always selects the finest one
	LOD_Distance,			// Based on a squared distance to camera
	LOD_ScreenSizeRelative,	// Based on a relative portion of screen occupied by rendered object
	LOD_ScreenSizeAbsolute	// Based on a number of pixels occupied by rendered object
};

class CView
{
protected:

	PRenderPath							RenderPath;
	CNodeAttrCamera*					pCamera; //???smart ptr?

	CArray<Scene::CNodeAttribute*>		VisibilityCache;
	bool								VisibilityCacheDirty; //???to flags?

	ELODType							MeshLODType;
	CFixedArray<float>					MeshLODScale;
	ELODType							MaterialLODType;
	CFixedArray<float>					MaterialLODScale;

public:

	//???add viewport settings here? to render multiple views into one RT

	//???scene start node? if NULL, render all nodes, else only that and its children
	Scene::CSPS*						pSPS;
	UI::PUIContext						UIContext;

	Render::PGPUDriver					GPU;
	CFixedArray<Render::PRenderTarget>	RTs;
	Render::PDepthStencilBuffer			DSBuffer;	//???or named? may require more than one in one view?
	Render::CEffectConstSetValues		Globals;

	CArray<Render::CRenderNode>			RenderQueue;	// Cached to avoid per-frame allocations

	CView(): pSPS(NULL), pCamera(NULL), VisibilityCacheDirty(true), MeshLODType(LOD_None), MaterialLODType(LOD_None) {}
	~CView();

	//visible lights cache (can separate by callback/visitor passed to SPS)
	//named/indexed texture RTs and mb named readonly system textures and named shader vars
	//!!!named resources in view bound to RP must be resolved by order number (index in array)
	//instead of looking up by name every time!
	//shadow cameras (?are generated from lights in a shadow phase?)
	//shadow map buffers (sort of RT / DS, no special case?)
	//materials for early depth, occlusion, shadows (?or in phases, predetermined?), or named materials?

	bool							SetRenderPath(CRenderPath* pNewRenderPath);
	CRenderPath*					GetRenderPath() const { return RenderPath.GetUnsafe(); }
	bool							SetCamera(CNodeAttrCamera* pNewCamera);
	const CNodeAttrCamera*			GetCamera() const { return pCamera; }
	void							UpdateVisibilityCache();
	CArray<Scene::CNodeAttribute*>&	GetVisibilityCache() { return VisibilityCache; } //???if dirty update right here?
	UPTR							GetMeshLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const;
	UPTR							GetMaterialLOD(float SqDistanceToCamera, float ScreenSpaceOccupiedRel) const;
	bool							RequiresScreenSize() const { return MeshLODType == LOD_ScreenSizeRelative || MeshLODType == LOD_ScreenSizeAbsolute || MaterialLODType == LOD_ScreenSizeRelative || MaterialLODType == LOD_ScreenSizeAbsolute; }
	bool							Render();
};

}

#endif
