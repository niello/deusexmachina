#pragma once
#include <Render/Renderer.h>

// Default renderer for CSkybox render objects.

namespace Render
{

class CSkyboxRenderer: public IRenderer
{
	FACTORY_CLASS_DECL;

public:

	CSkyboxRenderer();

	virtual bool                 Init(bool LightingEnabled, const Data::CParams& Params) override { OK; }
	virtual bool                 PrepareNode(IRenderable& Node, const CRenderNodeContext& Context) override;
	virtual CRenderQueueIterator Render(const CRenderContext& Context, CRenderQueue& RenderQueue, CRenderQueueIterator ItCurr) override;
};

}
