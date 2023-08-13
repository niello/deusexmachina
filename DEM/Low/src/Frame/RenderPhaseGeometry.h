#pragma once
#include <Frame/RenderPhase.h>
#include <Render/ShaderParamTable.h>
#include <Data/Dictionary.h>
#include <Data/FixedArray.h>
#include <map>

// Renders geometry batches, instanced when possible. Uses sorting, lights.
// Batches are designed to minimize shader state switches.

namespace Render
{
	using PRenderer = std::unique_ptr<class IRenderer>;
}

namespace Frame
{

class CRenderPhaseGeometry: public CRenderPhase
{
	FACTORY_CLASS_DECL;

protected:

	std::vector<U32>                                 _RenderQueueIndices;
	CFixedArray<CStrID>								 RenderTargetIDs;
	CStrID											 DepthStencilID;
	UPTR                                             _ShaderTechCacheIndex = 0;
	bool											 EnableLighting = false;

public:

	CRenderPhaseGeometry();
	virtual ~CRenderPhaseGeometry() override;

	virtual bool Init(CRenderPath& Owner, CGraphicsResourceManager& GfxMgr, CStrID PhaseName, const Data::CParams& Desc) override;
	virtual bool Render(CView& View) override;
};

}
