#pragma once
#ifndef __DEM_L1_FRAME_RENDER_PATH_H__
#define __DEM_L1_FRAME_RENDER_PATH_H__

#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Data/FixedArray.h>

// Render path incapsulates a full algorithm to render a frame, allowing to
// define it in a data-driven manner and therefore to avoid hardcoding frame rendering.
// It describes, how to use what shaders on what objects. The final output
// is a complete frame, rendered in an output render target.
// Render path consists of phases, each of which fills RT, MRT and/or DS,
// or does some intermediate processing, like an occlusion culling.
// Render path could be designed for some feature level (DX9, DX11), for some
// rendering concept (forward, deferred), for different features used (HDR) etc.

namespace Data
{
	class CParams;
}

namespace Resources
{
	class CRenderPathLoaderRP;
}

namespace Frame
{
class CView;
typedef Ptr<class CRenderPhase> PRenderPhase;

class CRenderPath: public Resources::CResourceObject
{
	__DeclareClassNoFactory;

protected:

	struct CRenderTargetSlot
	{
		vector4	ClearValue;
	};

	struct CDepthStencilSlot
	{
		U32		ClearFlags;
		float	DepthClearValue;
		U8		StencilClearValue;
	};

	CFixedArray<CRenderTargetSlot>			RTSlots;
	CFixedArray<CDepthStencilSlot>			DSSlots;

	CFixedArray<PRenderPhase>				Phases;

	Render::IShaderMetadata*				pGlobals;
	CFixedArray<Render::CEffectConstant>	Consts;
	CFixedArray<Render::CEffectResource>	Resources;
	CFixedArray<Render::CEffectSampler>		Samplers;

	friend class Resources::CRenderPathLoaderRP;

public:

	CRenderPath(): pGlobals(NULL) {}
	virtual ~CRenderPath();

	bool										Render(CView& View);

	virtual bool								IsResourceValid() const { return Phases.GetCount() > 0; } //???can be valid when empty?
	UPTR										GetRenderTargetCount() const { return RTSlots.GetCount(); }
	UPTR										GetDepthStencilBufferCount() const { return DSSlots.GetCount(); }
	const CFixedArray<Render::CEffectConstant>&	GetGlobalConstants() const { return Consts; }
	const Render::CEffectConstant*				GetGlobalConstant(CStrID Name) const;
	const Render::CEffectResource*				GetGlobalResource(CStrID Name) const;
	const Render::CEffectSampler*				GetGlobalSampler(CStrID Name) const;
};

typedef Ptr<CRenderPath> PRenderPath;

inline const Render::CEffectConstant* CRenderPath::GetGlobalConstant(CStrID Name) const
{
	UPTR i = 0;
	for (; i < Consts.GetCount(); ++i)
	{
		const Render::CEffectConstant& Curr = Consts[i];
		if (Curr.ID == Name) return &Curr;
	}
	return NULL;
}
//---------------------------------------------------------------------

inline const Render::CEffectResource* CRenderPath::GetGlobalResource(CStrID Name) const
{
	UPTR i = 0;
	for (; i < Resources.GetCount(); ++i)
	{
		const Render::CEffectResource& Curr = Resources[i];
		if (Curr.ID == Name) return &Curr;
	}
	return NULL;
}
//---------------------------------------------------------------------

inline const Render::CEffectSampler* CRenderPath::GetGlobalSampler(CStrID Name) const
{
	UPTR i = 0;
	for (; i < Samplers.GetCount(); ++i)
	{
		const Render::CEffectSampler& Curr = Samplers[i];
		if (Curr.ID == Name) return &Curr;
	}
	return NULL;
}
//---------------------------------------------------------------------

}

#endif
