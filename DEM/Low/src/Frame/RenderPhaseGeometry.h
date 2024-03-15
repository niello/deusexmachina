#pragma once
#include <Frame/RenderPhase.h>
#include <Data/FixedArray.h>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Frame
{

class CRenderPhaseGeometry: public CRenderPhase
{
	FACTORY_CLASS_DECL;

protected:

	std::vector<U32>    _RenderQueueIndices;
	CFixedArray<CStrID> _RenderTargetIDs; // TODO: could resolve into indices too?
	CStrID              _DepthStencilID; // TODO: could resolve into index too?
	UPTR                _ShaderTechCacheIndex = 0;

public:

	virtual bool Init(CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc) override;
	virtual bool Render(CView& View) override;
};

}
