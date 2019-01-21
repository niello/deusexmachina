#pragma once
#include <Resources/ResourceObject.h>
#include <Render/RenderFwd.h>
#include <Data/FixedArray.h>

// Render path incapsulates a full algorithm to render a frame, allowing to
// define it in a data-driven manner and therefore to avoid hardcoding frame rendering.
// It describes, how to use what shaders on what objects. The final output
// is a complete frame, rendered to an output render target.
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

	Render::IShaderMetadata*				pGlobals = nullptr;
	CFixedArray<Render::CEffectConstant>	Consts;
	CFixedArray<Render::CEffectResource>	Resources;
	CFixedArray<Render::CEffectSampler>		Samplers;

	friend class Resources::CRenderPathLoaderRP;

public:

	CRenderPath();
	virtual ~CRenderPath();

	bool										Render(CView& View);

	virtual bool								IsResourceValid() const { return Phases.GetCount() > 0; } //???can be valid when empty?
	UPTR										GetRenderTargetCount() const { return RTSlots.GetCount(); }
	UPTR										GetDepthStencilBufferCount() const { return DSSlots.GetCount(); }
	const CFixedArray<Render::CEffectConstant>&	GetGlobalConstants() const { return Consts; }
	const Render::CEffectConstant*				GetGlobalConstant(CStrID Name) const;
	const Render::CEffectResource*				GetGlobalResource(CStrID Name) const;
	const Render::CEffectSampler*				GetGlobalSampler(CStrID Name) const;

	void										SetRenderTargetClearColor(UPTR Index, const vector4& Color);
};

typedef Ptr<CRenderPath> PRenderPath;

}
